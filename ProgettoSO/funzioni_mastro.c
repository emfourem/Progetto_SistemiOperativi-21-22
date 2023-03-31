#include "libro_mastro.h"

int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;

void dealloca_tot(void){
	/*Dealloca semaforo e shm*/
	int lmid,sem_id;
	lmid = shmget(getpid(),0,SEMFLG);
	if(lmid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro.\n");
		exit(EXIT_FAILURE);
	}
	if((shmctl(lmid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del libro mastro.\n");
		exit(EXIT_FAILURE);
	}
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo del processo mastro.\n");
		exit(EXIT_FAILURE);
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo del processo mastro.\n");
		exit(EXIT_FAILURE);
	}
}
void dealloca_mastro(void){
	/*Dealloca shm*/
	int lmid;
	lmid = shmget(getpid(),0,SEMFLG);
	if(lmid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro.\n");
		exit(EXIT_FAILURE);
	}
	if((shmctl(lmid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del libro mastro.\n");
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
		printf("Errore nella wait for zero del libro mastro sul semaforo del master.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void reserve_sem(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=-1;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
	   	TEST_ERROR;
		printf("Errore nella reserve del libro mastro sul semaforo del master.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
int sem_find(void){
	int sem_id;
	if((sem_id=semget(getppid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del libro mastro per il semaforo del master.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	return sem_id;
}
int sem_crea(void){
	int sem_id;
	if((sem_id=semget(getpid(),1,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella creazione del semaforo del mastro.\n");
		dealloca_mastro();
		exit(EXIT_FAILURE);
	}
	return sem_id;
}
void inizializza_sem(int sem_id,int val){
	/*Semaforo di mutua esclusione*/
	union semun arg;
	if((semctl(sem_id,0,SETVAL,arg.val=val))<0){
		TEST_ERROR;
		printf("Errore nell'inizializzazione del semaforo del libro mastro.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void handle_sigusr2(int sig){
	dealloca_tot();
	exit(EXIT_SUCCESS);
}