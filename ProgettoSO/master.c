#include "master.h"


int main(int argc,char * argv[]){
	/*Dichiarazione variabili*/
	int i=0,shid,sem_id,*shared_data,pid,sem_prematuri;
	extern int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_REWARD,SO_RETRY,SO_TP_SIZE,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
	user *to_pid;
	struct sigaction sigint;
	struct sigaction sigalrm;
	struct sigaction sigusr;
	struct sigaction sigill;
	sigset_t mask_sigint;
	sigset_t mask_sigalrm;
	sigset_t mask_sigusr;
	sigset_t mask_sigill;
		if(argc>=10){
		SO_USERS_NUM=atoi(argv[1]);
		SO_NODES_NUM=atoi(argv[2]);
		SO_SIM_SEC=atoi(argv[3]);
		SO_BUDGET_INIT=atoi(argv[4]);
		SO_MIN_TRANS_GEN_NSEC=atol(argv[5]);
		SO_MAX_TRANS_GEN_NSEC=atol(argv[6]);
		SO_REWARD=atoi(argv[7]);
		SO_RETRY=atoi(argv[8]);
		SO_TP_SIZE=atoi(argv[9]);
		if(argc==12){
			SO_MIN_TRANS_PROC_NSEC=atol(argv[10]);
			SO_MAX_TRANS_PROC_NSEC=atol(argv[11]);
		}
		/*Gestione segnali*/
		sigemptyset(&mask_sigint);                 
		sigint.sa_mask = mask_sigint;
		sigint.sa_handler = handle_sigint;
		sigint.sa_flags = 0;
		sigaction(SIGINT, &sigint, NULL);
		sigemptyset(&mask_sigalrm);                  
		sigalrm.sa_mask = mask_sigalrm;
		sigalrm.sa_handler = handle_alarm;
		sigalrm.sa_flags = 0;
		sigaction(SIGALRM, &sigalrm, NULL);
		sigemptyset(&mask_sigusr);                
		sigusr.sa_mask = mask_sigusr;
		sigusr.sa_handler = handle_sigusr;
		sigusr.sa_flags = 0;
		sigaction(SIGUSR1, &sigusr, NULL);
		sigemptyset(&mask_sigill);                 
		sigill.sa_mask = mask_sigill;
		sigill.sa_handler = handle_sigill;
		sigill.sa_flags = 0;
		sigaction(SIGILL, &sigill, NULL);
		/*Creazione segmento di memoria condivisa per salvare i pid e poi la attacco*/
		shid = shmget(getpid(), sizeof(int)*(SO_USERS_NUM+SO_NODES_NUM+3),SEMFLG); /*MASTRO+n*UTENTI+m*NODI+for_write+prematuri+tot_send*/
		if(shid==-1){
			TEST_ERROR;
			printf("Errore nella creazione della memoria condivisa del master.\n");
			exit(EXIT_FAILURE);
		}
		shared_data=(int*)shmat(shid, NULL, 0);
		if(shared_data==(int*)-1){
			TEST_ERROR;
			printf("Errore nella shared memory attach nel master,dealloco la memoria creata e esco.\n");
			shmctl(shid,IPC_RMID,NULL);
			exit(EXIT_FAILURE);
		}
		pid = shmget(MYKEY, sizeof(user)*(SO_USERS_NUM),SEMFLG); /*Salvo i pid per fare le send*/
		if(pid==-1){
			TEST_ERROR;
			printf("Errore nella creazione della memoria condivisa per salvare i pid utenti del master,quindi dealloco la memoria condivisa creata in precedenza e esco.\n");
			shmctl(shid,IPC_RMID,NULL);
			exit(EXIT_FAILURE);
		}
		to_pid=(user*)shmat(pid, NULL, 0);
		if(to_pid==(user*)-1){
			TEST_ERROR;
			printf("Errore nella shared memory attach della memoria per i pid nel processo utente.\n");
			shmctl(shid,IPC_RMID,NULL);
			shmctl(pid,IPC_RMID,NULL);
			exit(EXIT_FAILURE);
		}
		sem_id=semget(getpid(),1,SEMFLG);
		if(sem_id==-1){
			TEST_ERROR;
			printf("Errore nella creazione del semaforo del master.\n");
			shmctl(shid,IPC_RMID,NULL);
			shmctl(pid,IPC_RMID,NULL);
			exit(EXIT_FAILURE);
		}
		sem_prematuri=semget(KEYPREM,1,SEMFLG);
		if(sem_prematuri==-1){
			TEST_ERROR;
			printf("Errore nella creazione del semaforo del master.\n");
			shmctl(shid,IPC_RMID,NULL);
			shmctl(pid,IPC_RMID,NULL);
			semctl(sem_id,0,IPC_RMID);
			exit(EXIT_FAILURE);
		}
		inizializza_mutex(sem_prematuri);
		inizializza_sem(sem_id);
		/*Inizializzazione*/
		*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1))=0;  /*Imposto il 'for write' del mastro*/
		*(shared_data+(SO_USERS_NUM+SO_NODES_NUM+1+1))=0;  /*Imposto i prematuri a 0*/
		/*Semaforo creato e inizializzato a SO_NODES_NUM*/
		/*Creazione processi utente e nodi*/
		for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
			switch(fork()){
				case -1:
					TEST_ERROR;
					printf("Errore nella fork del master.\n");
					dealloca_tot();
					exit(EXIT_FAILURE);
				case 0:
					/*Codice del figlio*/
					/*Inserisco il pid di tutti i processi figli a partire dal mastro poi utenti e infine i nodi nella memoria condivisa*/
					*(shared_data+(i))=getpid();
					if(i<1){
						if(execv("./libro_mastro",argv)==-1){
							TEST_ERROR;
							printf("Errore nella creazione del processo mastro.\n");
							dealloca_tot();
							exit(EXIT_FAILURE);
						}
					}else if(i<SO_USERS_NUM+1){
					/*Processi utente*/
						(*(to_pid+(i-1))).pid=getpid();
						if(execv("./user",argv)==-1){
							TEST_ERROR;
							printf("Errore nella creazione dei processi utente.\n");
							dealloca_tot();
							exit(EXIT_FAILURE);
						}
						/*I processi non torneranno più qui
						 *a meno di errori nell'execve
						 *quindi non serve ulteriore codice 
						 *se non la gestione di un eventuale errore
						 */

					}else {
						/*Processi nodi*/
						if(execv("./node",argv)==-1){
							TEST_ERROR;
							printf("Errore nella creazione dei processi nodo.\n");
							shmdt(shared_data);
							shmdt(to_pid);
							dealloca_tot();
							exit(EXIT_FAILURE);
						}
					}
				default:
					/*Codice del padre*/
					break;
			}
		}	
		wait_for_zero(sem_id); /*La simulazione parte quando è a 0 il semaforo*/
		alarm(SO_SIM_SEC);
		for(;;){
			stampa(shared_data);
			wait_nsecmaster();

		}
		exit(EXIT_SUCCESS);
	}
	exit(EXIT_FAILURE);
}













			