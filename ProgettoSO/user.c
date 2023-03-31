#include "user.h"


int main(int argc,char * argv[]){
	int tentativi=0,shid,*shared_data,node_to_send,c_id,sem_dad,prem,bilancio=0,i=0,pid,posizione,receiver,sem_prem;
	user *to_pid;
	extern int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
	struct sigaction sigusr;
	struct sigaction sigusr_one;
	struct sigaction sigquit;
	sigset_t mask_sigusr;
	sigset_t mask_sigusr1;
	sigset_t mask_sigquit;
	struct mymsg send;
	SO_USERS_NUM=atoi(argv[1]);
	SO_NODES_NUM=atoi(argv[2]);
	SO_SIM_SEC=atoi(argv[3]);
	SO_BUDGET_INIT=atoi(argv[4]);
	SO_MIN_TRANS_GEN_NSEC=atoi(argv[5]);
	SO_MAX_TRANS_GEN_NSEC=atoi(argv[6]);
	SO_REWARD=atoi(argv[7]);
	SO_RETRY=atoi(argv[8]);
	SO_TP_SIZE=atoi(argv[9]);
	if(argc==12){
		SO_MIN_TRANS_PROC_NSEC=atol(argv[10]);
		SO_MAX_TRANS_PROC_NSEC=atol(argv[11]);
	}
	/*Gestione segnali*/
	sigemptyset(&mask_sigusr);
	sigusr.sa_mask = mask_sigusr;
	sigusr.sa_handler = handle_sigusr2;
	sigusr.sa_flags = 0;
	sigaction(SIGUSR2, &sigusr, NULL);
	sigemptyset(&mask_sigusr1);
	sigusr_one.sa_mask = mask_sigusr1;
	sigusr_one.sa_handler = handle_sigusr1;
	sigusr_one.sa_flags = 0;
	sigaction(SIGUSR1, &sigusr_one, NULL);
	sigemptyset(&mask_sigquit);
	sigquit.sa_mask = mask_sigquit;
	sigquit.sa_handler = handle_sigquit;
	sigquit.sa_flags = 0;
	sigaction(SIGQUIT, &sigquit, NULL);       /*In risposta genero una transazione*/
	srand(getpid());
	/*Creazione e collegamenti per memorie condivise*/
	shid = shmget(getppid(),0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa.\n");
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nell'attach alla memoria condivisa dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	pid= shmget(MYKEY,0,SEMFLG);
	if(pid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa degli utenti.\n");
		exit(EXIT_FAILURE);
	}
	to_pid=(user*)shmat(pid, NULL, 0);
	if(to_pid==(user*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa degli utenti di %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	sem_prem=sem_find(KEYPREM);
	/*Inizializzazione variabili*/
	for(;i<SO_USERS_NUM;i++){
		if((*(to_pid+(i))).pid==getpid()){posizione=i;}
	}
	i=0;
	/*Wait for zero per il semaforo del padre*/
	sem_dad=sem_find(getppid());
	reserve_sem(sem_dad);
	wait_for_zero(sem_dad);
	/*Parto quando è a zero*/
	while(tentativi<SO_RETRY){
		if((bilancio=(calc_bilancio((*(to_pid+(posizione))).money,*(shared_data))))>=2){
			/*Pid del nodo a cui inviare i messaggi*/
			node_to_send=*(shared_data+(estrai_random(SO_USERS_NUM+1,SO_NODES_NUM+SO_USERS_NUM)));
			/*Mi collego alla coda,invio un messaggio(non uso mutua esclusione) e salvo ogni volta gli importi spediti*/
			c_id=coda_find(node_to_send);
			send.mtype=node_to_send;
			receiver=estrai_random(0,SO_USERS_NUM);
			while(receiver==posizione||(*(to_pid+(receiver))).pid==0){
				receiver=estrai_random(0,SO_USERS_NUM);
			}
			/*Così non estraggo pid di processi già morti e neanche il processo stesso*/
			send.t=genera(bilancio,(*(to_pid+(receiver))).pid); /*Passo:index,collegamento alla mem.condivisa,bilancio calcolato,receiver estratto a caso*/
			(*(to_pid+(posizione))).money+=send.t.money;
			if((msgsnd(c_id,&send,sizeof(send.t),0))<0){
				TEST_ERROR;
				printf("Errore nella message send dell'utente %d.\n",getpid());
				exit(EXIT_FAILURE);
			}
			/*Simulo l'elaborazione*/
			wait_nsec();
			/*Riparto*/
		}else{
			/*Attesa di un determinato tempo estratto in nanosecondi se il bilancio è inferiore a due
			  Poi incremento i tentativi e ricalcolo il bilancio
			*/
			wait_nsec();
			tentativi+=1;
			if(tentativi==SO_RETRY){
				(*(to_pid+(posizione))).pid=0; /*Così gli altri sanno che ho finito*/
				prem=*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1+1));
				if(prem==SO_USERS_NUM-2){ /*Non ci sono più processi vivi o meglio ce n'è solo uno che non può inviare transazioni*/
					*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1+1))=prem+2;
					if((shmdt(shared_data))<0){
						TEST_ERROR;
						printf("Errore nella detach della memoria condivisa dell'utente %d.\n",getpid());
						exit(EXIT_FAILURE);
					}
					if((shmdt(to_pid))<0){
						TEST_ERROR;
						printf("Errore nella detach della memoria condivisa degli utenti dell'utente %d.\n",getpid());
						exit(EXIT_FAILURE);
					}
					kill(getppid(),SIGILL);
					exit(EXIT_FAILURE);
				}else{
					reserve_sem(sem_prem);
					*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1+1))=prem+1;
					release_sem(sem_prem);
					if((shmdt(shared_data))<0){
						TEST_ERROR;
						printf("Errore nella detach della memoria condivisa dell'utente %d.\n",getpid());
						exit(EXIT_FAILURE);
					}
					if((shmdt(to_pid))<0){
						TEST_ERROR;
						printf("Errore nella detach della memoria condivisa degli utenti dell'utente %d.\n",getpid());
						exit(EXIT_FAILURE);
					}
					exit(EXIT_FAILURE);
				}
			}
			/*Poi incremento i tentativi e ricalcolo il bilancio*/
		}
	}
	exit(EXIT_SUCCESS);	
}