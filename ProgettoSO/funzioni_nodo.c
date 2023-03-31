#include "node.h"

int remaining,pausa,SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
transaction vuota(void){
	transaction t;
	/*Creo transazione vuota per pulire i blocchi scritti*/
	t.timestamp=0;
	t.sender=0;
	t.receiver=0;
	t.money=0;
	t.reward=0;
	return t;
}
void dealloca_coda(void){
	int n_id;
	if ((n_id=msgget(getpid(),0))<0) {
		TEST_ERROR;
		printf("Errore nella ricerca della coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
 	}
	if ((msgctl(n_id,IPC_RMID,NULL))<0) {
		TEST_ERROR;
		printf("Errore nella rimozione della coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
 	}
}
int estrai_random(int estremo_sx,int estremo_dx){
	return (rand()% (estremo_dx-estremo_sx+1))+estremo_sx;
}
int crea_coda(void){
	int n_id;
	if ((n_id=msgget(getpid(),SEMFLG))<0) {
		TEST_ERROR;
		printf("Errore nella creazione della coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
 	}
	return n_id;
}
transaction genera_transazione(int reward){
	transaction ret;
	struct timespec spec; /*definito in time per timestamp*/
	if((clock_gettime(CLOCK_MONOTONIC,&spec))<0){
		TEST_ERROR;
		printf("Errore nel richiamo della clock_gettime del nodo %d.\n",getpid());
		dealloca_coda();
		exit(EXIT_FAILURE);
	}
	ret.timestamp=(spec.tv_sec*NANO )+spec.tv_nsec ; /*Risoluzione dei nanosecondi*/
	ret.sender=SENDER;
	ret.receiver=getpid();
	ret.money=reward;
	ret.reward=0;
	return ret;
}

void handle_sigusr2(int sig){
	int c_id;
	struct msqid_ds my_q_data;
	if ((c_id=msgget(getpid(),0))<0) {
		TEST_ERROR;
		printf("Errore nella ricerca della coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
 	}
	if((msgctl(c_id, IPC_STAT, &my_q_data))<0){
		TEST_ERROR;
		printf("Errore nella msgctl sulla coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	printf("Ci sono %ld messaggi in coda di %d\n",my_q_data.msg_qnum+remaining,getpid());
	if((msgctl(c_id,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della coda del nodo %d.\n",getpid());
		exit(EXIT_FAILURE);
	}
	/*Coda eliminata*/
	exit(EXIT_SUCCESS);
}
void reserve_sem(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=-1;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
	    TEST_ERROR;
		printf("Errore nella reserve del nodo %d.\n",getpid());
		dealloca_coda();
		exit(EXIT_FAILURE);
	}
}
void wait_for_zero(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=0;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
		TEST_ERROR;
		printf("Errore nella wait for zero sul semaforo del master del nodo %d.\n",getpid());
		dealloca_coda();
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
		printf("Errore nella release del nodo %d.\n",getpid());
		dealloca_coda();
		exit(EXIT_FAILURE);
	}
}
int sem_find(int destinatario){
	int sem_id;
	if((sem_id=semget(destinatario,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo del nodo %d.\n",getpid());
		dealloca_coda();
		exit(EXIT_FAILURE);
	}
	return sem_id;
}
void wait_nsec_block(void){
	if(pausa==1){
		struct timespec wait_tran;
		/*
		struct timespec {
			time_t   tv_sec;        //seconds 
			long     tv_nsec;       //nanoseconds 
		};*/
		wait_tran.tv_sec=0;
		wait_tran.tv_nsec=estrai_random(SO_MAX_TRANS_PROC_NSEC,SO_MIN_TRANS_PROC_NSEC);
		/*La nanosleep durerà tv_sec secondi e tv_nsec nanosecondi*/
			if (nanosleep(&wait_tran, NULL) == -1) {
				switch (errno) {
				case EINTR:
					printf("nanosleep interrupted by a signal handler\n");
				case EINVAL:
					printf("tv_nsec - not in range or tv_sec is negative\n");
				default:
					printf("nanosleep");
				}
				errno=0;/*Non esco ma gestisco l'errore quando una transazione viene scartata*/
			}
	}else{
		struct timespec wait_tran;
		/*
		struct timespec {
			time_t   tv_sec;        //seconds 
			long     tv_nsec;       //nanoseconds 
		};*/
		wait_tran.tv_sec=0;
		wait_tran.tv_nsec=0;
		/*La nanosleep durerà tv_sec secondi e tv_nsec nanosecondi*/
		errno=0;
		if (nanosleep(&wait_tran, NULL) == -1) {
			switch (errno) {
			case EINTR:
				printf("nanosleep interrupted by a signal handler\n");
			case EINVAL:
				printf("tv_nsec - not in range or tv_sec is negative\n");
			default:
				printf("nanosleep");
			}
			errno=0;/*Non esco ma gestisco l'errore quando una transazione viene scartata*/
		}
	}
	
}
