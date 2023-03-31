#include "node.h"

int main(int argc,char * argv[]){
	int shid,*shared_data,for_write,get_val=0,lib_id,to_mastro,reward=0,i=0,e,sem_dad,n_id,pid,j=0;
	blocco	*libro_mastro,to_send;
	user *to_pid;
	transaction t;
	transaction *c;
	extern int remaining,pausa,SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
	struct mymsg receive;
	struct msqid_ds my_q_data;
	struct sigaction sigusr;
	sigset_t mask_sigusr;
	SO_USERS_NUM=atoi(argv[1]);
	SO_NODES_NUM=atoi(argv[2]);
	SO_SIM_SEC=atoi(argv[3]);
	SO_BUDGET_INIT=atoi(argv[4]);
	SO_MIN_TRANS_GEN_NSEC=atoi(argv[5]);
	SO_MAX_TRANS_GEN_NSEC=atoi(argv[6]);
	SO_REWARD=atoi(argv[7]);
	SO_RETRY=atoi(argv[8]);
	SO_TP_SIZE=atoi(argv[9]);
	pausa=0; /*Se argc==12 o meno*/
	if(argc==12){
		SO_MIN_TRANS_PROC_NSEC=atol(argv[10]);
		SO_MAX_TRANS_PROC_NSEC=atol(argv[11]);
		pausa=1;
	}
	remaining=0;
	srand(getpid());	
	/*Gestione segnali*/
	sigemptyset(&mask_sigusr);
	sigusr.sa_mask = mask_sigusr;
	sigusr.sa_handler = handle_sigusr2;
	sigusr.sa_flags = 0;
	sigaction(SIGUSR2, &sigusr, NULL);
	/*Collegamenti a memorie condivise e inizializzazione coda*/
	t=vuota();
	shid = shmget(getppid(),0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	pid= shmget(MYKEY,0,SEMFLG);
	if(pid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa degli utenti del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	to_pid=(user*)shmat(pid, NULL, 0);
	if(to_pid==(user*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa degli utenti del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	n_id=crea_coda();
	/*Fine preparazione,trovo il semaforo del master faccio il decremento e la wait for zero*/
	sem_dad=sem_find(getppid());
	reserve_sem(sem_dad);
	wait_for_zero(sem_dad);
	lib_id=sem_find(*(shared_data));
	to_mastro= shmget(*(shared_data),0,SEMFLG);
	if(to_mastro==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	libro_mastro=(blocco *)shmat(to_mastro, NULL, 0);
	if(libro_mastro==(blocco*)-1){
		TEST_ERROR;
		printf("Errore nell'attach del libro mastro del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	while(1){
		while(get_val<SO_BLOCK_SIZE-1){
			if((msgctl(n_id, IPC_STAT, &my_q_data))<0){
				TEST_ERROR;
				printf("Errore nella msgctl del nodo %d.\n",getpid());
				dealloca_coda();
				exit(EXIT_FAILURE);
			}
			get_val=my_q_data.msg_qnum;
		}
		if(get_val<=SO_TP_SIZE){	
			reward=0;
			e=0;
			while(e<SO_BLOCK_SIZE-1){
				if((msgrcv(n_id,&receive,sizeof(receive.t),0,MSG_NOERROR))<0){
					TEST_ERROR;
					printf("Errore nella msgrcv del nodo %d.\n",getpid());
					dealloca_coda();
					exit(EXIT_FAILURE);
				}
				to_send.block[e]=receive.t;
				reward+=receive.t.reward;
				e++;
			}
			to_send.block[e]=genera_transazione(reward);
			/*Qui bisogna scrivere sul libro mastro dato che il blocco è stato scritto in maniera corretta*/
			reserve_sem(lib_id);
			/*Accedo al mastro in mutua esclusione*/
			for_write=*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1)); /*valore vecchio da usare come indice del blocco nel mastro */
				if(for_write<SO_REGISTRY_SIZE){ /*minore di registry_size visto che sarà terminato dal mastro*/
					/*Deve essere atomica,se accedo qui devo poter scrivere sul mastro ecco perchè il controllo sul <SO_REGISTRY_SIZE*/
					to_send.id=for_write;
					wait_nsec_block();/*Simula l'elaborazione*/
					*(libro_mastro+for_write)=to_send;
					*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1))=for_write+1;/*Aggiorno il valore dell'indice dato che ho scritto*/
					if(for_write+1==SO_REGISTRY_SIZE){
						kill(getppid(),SIGUSR1);
					}
				}
			release_sem(lib_id);
			e=0;
			while(e<SO_BLOCK_SIZE){
				to_send.block[e++]=t;
			}
			/*Ho svuotato il blocco e riparto*/
		}else{
			/*Transaction pool piena*/
			c=(transaction *) malloc(SO_TP_SIZE*sizeof(transaction));
			i=0;
			while(i!=SO_TP_SIZE){
				if((msgrcv(n_id,&receive,sizeof(receive.t),0,MSG_NOERROR))<0){
					TEST_ERROR;
					printf("Errore nella msgrcv del nodo %d.\n",getpid());
					dealloca_coda();
					exit(EXIT_FAILURE);
				}
				*(c+i)=receive.t;
				i++;
			}
			remaining=SO_TP_SIZE;
			/*Mi salvo tutte le 100 transazioni più vecchie*/
			while(get_val!=0){
				if((msgrcv(n_id,&receive,sizeof(receive.t),0,MSG_NOERROR))<0){
					TEST_ERROR;
					printf("Errore nella msgrcv del nodo %d.\n",getpid());
					dealloca_coda();
					exit(EXIT_FAILURE);
				}
				i=0;
				for(;i<SO_USERS_NUM;i++){
					if((*(to_pid+(i))).pid==receive.t.sender){
						(*(to_pid+(i))).money-=receive.t.money;
					}
				}
				kill(receive.t.sender,SIGUSR1);
				if((msgctl(n_id, IPC_STAT, &my_q_data))<0){
					TEST_ERROR;
					printf("Errore nella msgctl del nodo %d.\n",getpid());
					dealloca_coda();
					exit(EXIT_FAILURE);
				}
				get_val=my_q_data.msg_qnum;
			}
			/*Esco quando ho scartato e processo tutto quello che avevo nella tp*/
			i=0;
			j=0;
			for(;i<SO_TP_SIZE/(SO_BLOCK_SIZE-1);i++){
				reward=0;
				e=0;
				while(e<SO_BLOCK_SIZE-1){
					to_send.block[e]=*(c+j);
					reward+=(c+j)->reward;
					*(c+j)=t;
					e++;
					j++;
				}
				to_send.block[e]=genera_transazione(reward);
				/*Qui bisogna scrivere sul libro mastro dato che il blocco è stato scritto in maniera corretta*/
				reserve_sem(lib_id);
				/*Accedo al mastro in mutua esclusione*/
				for_write=*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1)); /*valore vecchio da usare come indice del blocco nel mastro */
					if(for_write<SO_REGISTRY_SIZE){ /*minore di registry_size visto che sarà terminato dal mastro*/
						/*Deve essere atomica,se accedo qui devo poter scrivere sul mastro ecco perchè il controllo sul <SO_REGISTRY_SIZE*/
						to_send.id=for_write;
						wait_nsec_block();/*Simula l'elaborazione*/
						*(libro_mastro+for_write)=to_send;
						remaining-=SO_BLOCK_SIZE-1;
						*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1))=for_write+1;/*Aggiorno il valore dell'indice dato che ho scritto*/
						if(for_write+1==SO_REGISTRY_SIZE){
							kill(getppid(),SIGUSR1);
						}
					}
				release_sem(lib_id);
				e=0;
				while(e<SO_BLOCK_SIZE){
					to_send.block[e++]=t;
				}
			}
			free(c);
		}
	}
}