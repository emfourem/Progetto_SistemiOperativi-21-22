#include "user.h"

int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
int find_index(int* shared){
	int i=1;
	for(;i<SO_USERS_NUM+1;i++){
		if((*(shared+(i)))==getpid()){return i;}
		i++;
	}
	return -1;
}
int estrai_random(int estremo_sx,int estremo_dx){
	return (rand()% (estremo_dx-estremo_sx+1))+estremo_sx;
}
void wait_nsec(void){
	struct timespec wait_tran;
	/*
	struct timespec {
		time_t   tv_sec;        //seconds 
		long     tv_nsec;       //nanoseconds 
	};*/
	wait_tran.tv_sec=0;
	wait_tran.tv_nsec=estrai_random(SO_MAX_TRANS_GEN_NSEC,SO_MIN_TRANS_GEN_NSEC);
	/*La nanosleep durerà tv_sec secondi e tv_nsec nanosecondi*/
		if (nanosleep(&wait_tran, NULL) == -1) {
			/*Gestisco così nel caso in cui la nanosleep dovesse essere interrotta da un segnale e in preciso SIGQUIT*/
			errno=0;
			/*Metto errno a 0 e continuo ad eseguire*/
		}
}
void wait_for_zero(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=0;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
		TEST_ERROR;
		printf("Errore nella wait for zero sul semaforo del master dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	
}
int sem_find(int chiave){
	int sem_id;
	if((sem_id=semget(chiave,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo del master dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	return sem_id;
}
void reserve_sem(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=-1;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
	    TEST_ERROR;
		printf("Errore nella reserve sul semaforo del master dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
}
void release_sem(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=1;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
	    TEST_ERROR;
		printf("Errore nella release sul semaforodell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
}
int calc_bilancio(int money,int pid_mastro){
	int to_mastro,i=0,j=0,ricevuti=0,pagati=0,q;
	blocco x,* libro_mastro;
	transaction y;
	/*Passo il pid del mastro per accedere al mastro*/
	to_mastro = shmget(pid_mastro,0,SEMFLG);
	if(to_mastro==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	libro_mastro=(blocco *)shmat(to_mastro, NULL, 0);
	if(libro_mastro==(blocco*)-1){
		TEST_ERROR;
		printf("Errore nell'attach del libro mastro dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	/*Collegato al libro mastro
	Ora inizio a leggere cercando le transazioni nei blocchi in cui l'utente è receiver e sender*/
	for(;i<SO_REGISTRY_SIZE;i++){
		x=*(libro_mastro+i);  /*Puntatore al primo elemento dell'array*/
		for(;j<SO_BLOCK_SIZE;j++){
			y=x.block[j];
			if(y.timestamp==0){
				i=SO_REGISTRY_SIZE;
			}else if(y.sender==getpid()){
				pagati+=y.money;
			}else if(y.receiver==getpid()){
				ricevuti+=y.money;
			}else{
				
			}
		}
		j=0;
	}
	/*Se esco è stato scritto in tutte le posizioni*/
	q=SO_BUDGET_INIT+(ricevuti-pagati)-(money-pagati);
	if((shmdt(libro_mastro))<0){
		TEST_ERROR;
		printf("Errore nella detach del libro mastro dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	return q;
	
}
transaction genera(int bilancio,int receiver){
	transaction ret;
	int rand;
	struct timespec spec; /*definito in time per timestamp*/
	if((clock_gettime(CLOCK_MONOTONIC,&spec))<0){
		/*CLOCK MONOTONIC:orologio che non può essere impostato e rappresenta il tempo monotono da un punto di partenza non specificato. */
		TEST_ERROR;
		printf("Errore nel richiamo della clock_gettime nel processo utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	ret.timestamp=(spec.tv_sec*NANO )+spec.tv_nsec ; /*Risoluzione dei nanosecondi*/
	ret.sender=getpid();
	ret.receiver=receiver;
	rand=estrai_random(2,bilancio);
	ret.reward=(SO_REWARD*rand)/100;
	if(ret.reward<=0){ret.reward=1;}
	ret.money=rand-ret.reward;
	return ret;
}
int coda_find(int destinatario){
	int c_id;
	if ((c_id=msgget(destinatario,0))<0) {
		TEST_ERROR;
		printf("Errore nella ricerca della coda privata del processo nodo %d dell'utente %d.\n",destinatario,getpid());
		exit(EXIT_FAILURE);
	}
	return c_id;
}
void handle_sigquit(int sig){
	int node_to_send,c_id,shid,*shared_data,pid,receiver,posizione,bilancio,i=0;
	user *to_pid;
	struct mymsg send_signal;
	shid = shmget(getppid(), 0 ,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	pid= shmget(MYKEY,0,SEMFLG);
	if(pid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa degli utenti. Utente: %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	to_pid=(user*)shmat(pid, NULL, 0);
	if(to_pid==(user*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa degli utenti. Utente: %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	for(;i<SO_USERS_NUM;i++){
		if(getpid()==(to_pid+(i))->pid){
			posizione=i;
		    i=SO_USERS_NUM;
		}
	}
	if((bilancio=calc_bilancio((*(to_pid+(posizione))).money,*(shared_data)))>=2){
		node_to_send=*(shared_data+(estrai_random(SO_USERS_NUM+1,SO_NODES_NUM+SO_USERS_NUM))); 
		c_id=coda_find(node_to_send);
		send_signal.mtype=node_to_send;
		receiver=estrai_random(0,SO_USERS_NUM);
		while((*(to_pid+(receiver))).pid==getpid()||(*(to_pid+(receiver))).pid==0){ /*Non è lui stesso il receiver e non è terminato*/
			receiver=estrai_random(0,SO_USERS_NUM);
		}
		send_signal.t=genera(bilancio,(*(to_pid+(receiver))).pid);
		if((msgsnd(c_id,&send_signal,sizeof(send_signal.t),0))<0){
			TEST_ERROR;
			printf("Errore nella message send dell'utente %d.\n",getpid());
			exit(EXIT_FAILURE);
		}
		(*(to_pid+(posizione))).money+=send_signal.t.money;
		printf("Transazione generata ed inviata con successo dopo la ricezione del segnale.\n");
	}else{
		printf("Impossibile generare transazione dato che il bilancio del processo è: %d.\n",bilancio);
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
		printf("Errore nella detach della memoria condivisa dell'utente %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	if((shmdt(to_pid))<0){
		TEST_ERROR;
		printf("Errore nella detach della memoria condivisa degli utenti. Utente: %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
}
void handle_sigusr2(int sig){	
	exit(EXIT_SUCCESS);
}
void handle_sigusr1(int sig){
	/*Avviso transazione scartata*/
	printf("Transazione scartata,l'utente %d ha ripreso il denaro inviato.\n",getpid());
}