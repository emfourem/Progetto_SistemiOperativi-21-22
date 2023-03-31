#include "libro_mastro.h"

int main(int argc,char * argv[]){
	/*Dichiarazione variabili*/
	int lmid,sem_id,sem_dad;
	blocco *libro_mastro;
	extern int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
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
	/*Creazione libro mastro e risorse per la gestione del libro mastro(semaforo per la mutua esclusione e ricerca del semaforo del master per fare la wait for zero)*/
	lmid = shmget(getpid(), sizeof(blocco)*SO_REGISTRY_SIZE,SEMFLG);
	if(lmid==-1){
		TEST_ERROR;
		printf("Errore nella creazione del libro mastro.\n");
		exit(EXIT_FAILURE);
	}
	libro_mastro=(blocco*)shmat(lmid, NULL,0);
	if(libro_mastro==(blocco*)-1){
		TEST_ERROR;
		printf("Errore nell'attach del libro mastro.\n");
		shmctl(lmid,IPC_RMID,NULL);
		exit(EXIT_FAILURE);
	}
	sem_id=sem_crea();
	inizializza_sem(sem_id,1);
	sem_dad=sem_find();
	reserve_sem(sem_dad);
	wait_for_zero(sem_dad); /*Parte qui la simulazione->dopo la wait for zero*/
	pause();
	exit(EXIT_SUCCESS);
}