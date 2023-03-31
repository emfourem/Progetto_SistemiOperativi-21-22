#include "master.h"

int SO_USERS_NUM,SO_NODES_NUM,SO_SIM_SEC,SO_RETRY,SO_TP_SIZE,SO_REWARD,SO_BUDGET_INIT,SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC,SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;

void dealloca_tot(void){
	int i=0,shid,*shared_data,sem_id,pid,sem_prem;
	shid = shmget(getpid(), 0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa del master.\n");
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nell'attach della memoria condivisa del master.\n");
	}
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		if((*(shared_data+(i)))!=0){
			if((kill(*(shared_data+(i)),SIGUSR2))<0){
				/*Lancio segnale ai processi così che possano deallocare le risorse che hanno creato se l'hanno fatto*/
				TEST_ERROR;
				printf("Processo già terminato.\n");
			}
		}
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
		printf("Errore nella detach della memoria condivisa del master.\n");
	}
	if((shmctl(shid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa del master.\n");
	}
	pid = shmget(MYKEY,0,SEMFLG); 
	if(pid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa degli utenti nel master.\n");
	}
	if((shmctl(pid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa degli utenti nel master.\n");
	}
	
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dall'utente.\n");
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dall'utente.\n");
	}
	if((sem_prem=semget(KEYPREM,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dall'utente.\n");
	}
	if((semctl(sem_prem,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dall'utente.\n");
	}
	printf("Deallocazione terminata.\n");
	exit(EXIT_FAILURE);
}
void stampa(int * shared){	
	/*Vado nel mastro e leggo da lì i money inviati e ricevuti e in più i reward dei singoli nodi*/
	int to_mastro,i=0,j=0,k=1,budget_utente=SO_BUDGET_INIT,budget_nodo=0,pid,q=0,p=0; /*k=1 perchè evito il pid del mastro così*/
	blocco *libro_mastro,x;
	transaction y;
	int stamp[2][3];
	int stampm[2][3];
	while(q<3){
		stamp[0][q]=0;
		stamp[1][q]=0;
		stampm[0][q]=0;
		stampm[1][q]=SO_BUDGET_INIT;
		q++;
	}
	q=0;
	to_mastro = shmget(*(shared),0,SEMFLG);
	if(to_mastro==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro nel metodo per la stampa.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	libro_mastro=(blocco *)shmat(to_mastro, NULL, 0);
	if(libro_mastro==(blocco*)-1){
		TEST_ERROR;
		printf("Errore nel libro mastro attach nel metodo per la stampa.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	for(;k<(SO_USERS_NUM+SO_NODES_NUM+1);k++){
		pid=*(shared+(k));
		if(k<=SO_USERS_NUM){
			i=0;
			for(;i<SO_REGISTRY_SIZE;i++){ 
				x=*(libro_mastro+i);
				for(;j<SO_BLOCK_SIZE;j++){
					y=x.block[j];
						if(y.timestamp!=0){
							if(y.sender==pid){
								budget_utente-=y.money;	
							}else if(y.receiver==pid){
								budget_utente+=y.money;
							}else{
							}
						}else{
							j=SO_BLOCK_SIZE;
							i=SO_REGISTRY_SIZE;
							/*Così non entro più nel for della transaction e neanche nell'altro visto che non c'è nulla nel libro mastro ancora*/
						}
				}
				j=0;
			}
			/*printf("Il budget del processo utente %d è %d.\n",pid,budget_utente);*/
			while(q<3){
				if(stamp[1][q]<budget_utente){
					stamp[1][q]=budget_utente;
					stamp[0][q]=pid;
					q=3;
				}else if(stampm[1][q]>budget_utente){
					stampm[1][q]=budget_utente;
					stampm[0][q]=pid;
					q=3;
				}else{
					q++;
				}
			}
			q=0;
			budget_utente=SO_BUDGET_INIT;
		}else{ /*Nodo*/
			p=0;
			while(q<6){
				if(q<3){
					printf("Il budget del processo utente %d è %d.\n",stamp[0][q],stamp[1][q]);
					q++;
				}else{
					printf("Il budget del processo utente %d è %d.\n",stampm[0][p],stampm[1][p]);
					q++;
					p++;
				}
			}
			i=0;
			for(;i<SO_REGISTRY_SIZE;i++){
				x=*(libro_mastro+i);
				y=x.block[SO_BLOCK_SIZE-1];
				if(y.timestamp!=0){
					if(y.receiver==pid){
						budget_nodo+=y.money;
					}
				}else{
					i=SO_REGISTRY_SIZE;
					/*Così esco dato che il mastro sarà vuoto*/
				}
			}
			printf("Il budget del processo nodo %d è %d.\n",pid,budget_nodo);
			budget_nodo=0;
		}
	}
	if((shmdt(libro_mastro))<0){
		TEST_ERROR;
		printf("Errore nel libro mastro detach nel metodo per la stampa finale.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void stampa_finale(int * shared){
	/*Vado nel mastro e leggo da lì i money inviati e ricevuti e in più i reward dei singoli nodi*/
	int to_mastro,i=0,j=0,k=1,budget_utente=SO_BUDGET_INIT,budget_nodo=0,pid; /*k=1 perchè evito il pid del mastro così*/
	blocco *libro_mastro,x;
	transaction y;
	to_mastro = shmget(*(shared),0,SEMFLG);
	if(to_mastro==-1){
		TEST_ERROR;
		printf("Errore nel collegamento al libro mastro nel metodo per la stampa finale.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	libro_mastro=(blocco *)shmat(to_mastro, NULL, 0);
	if(libro_mastro==(blocco*)-1){
		TEST_ERROR;
		printf("Errore nel libro mastro attach nel metodo per la stampa finale.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	for(;k<(SO_USERS_NUM+SO_NODES_NUM+1);k++){
		pid=*(shared+(k));
		if(k<=SO_USERS_NUM){
			i=0;
			for(;i<SO_REGISTRY_SIZE;i++){ 
				x=*(libro_mastro+i);
				for(;j<SO_BLOCK_SIZE;j++){
					y=x.block[j];
						if(y.timestamp!=0){
							if(y.sender==pid){
								budget_utente-=y.money;	
							}else if(y.receiver==pid){
								budget_utente+=y.money;
							}else{
							}
						}else{
							j=SO_BLOCK_SIZE;
							i=SO_REGISTRY_SIZE;
							/*Così non entro più nel for della transaction e neanche nell'altro visto che non c'è nulla nel libro mastro ancora*/
						}
				}
				j=0;
			}
			printf("Il budget del processo utente %d è %d.\n",pid,budget_utente);
			budget_utente=SO_BUDGET_INIT;
		}else{ /*Nodo*/
			i=0;
			for(;i<SO_REGISTRY_SIZE;i++){
				x=*(libro_mastro+i);
				y=x.block[SO_BLOCK_SIZE-1];
				if(y.timestamp!=0){
					if(y.receiver==pid){
						budget_nodo+=y.money;
					}
				}else{
					i=SO_REGISTRY_SIZE;
					/*Così esco dato che il mastro sarà vuoto*/
				}
			}
			printf("Il budget del processo nodo %d è %d.\n",pid,budget_nodo);
			budget_nodo=0;
		}
	}
	if((shmdt(libro_mastro))<0){
		TEST_ERROR;
		printf("Errore nel libro mastro detach nel metodo per la stampa finale.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	printf("I blocchi correttamente nel mastro sono %d\n",*(shared+(SO_USERS_NUM+SO_NODES_NUM+1)));
	printf("I processi utente prematuri sono %d\n",*(shared+(SO_USERS_NUM+SO_NODES_NUM+1+1)));
}
void handle_alarm(int sig){
	int shid,*shared_data,i=0,status,sem_id,mem_user,sem_prem;
	pid_t pid;
	printf("Segnale di sigalrm gestito correttamente. Terminazione dovuta all'allarme dato che sono passati %d secondi. Procediamo con la stampa data la terminazione della simulazione: \n",SO_SIM_SEC);
	shid = shmget(getpid(), 0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa nella gestione della terminazione dovuta al raggiungimento del limite massimo di tempo.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nella shared memory attach per gestire il segnale SIGALRM.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	i=1; /*Inizialmente non invio nulla al mastro*/
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		if((kill(*(shared_data+(i)),SIGUSR2))<0){
			TEST_ERROR;
			printf("Errore nella kill del processo %d nella gestione del segnale SIGALRM .\n",*(shared_data+(i)));
			dealloca_tot();
			exit(EXIT_FAILURE);
		}
	}
	i=1;
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		while ((pid=waitpid((*(shared_data+(i))), &status,0))!= -1) {
			/*Controllo e aspetto che tutti siano terminati*/
		}
	}
	/*Stampo e controllo blocchi su mastro e prematuri*/
	stampa_finale(shared_data);
	/*Uccido il mastro dopo che ho letto quindi può deallocare e uscire*/
	if((kill(*(shared_data),SIGUSR2))<0){
		TEST_ERROR;
		printf("Errore nella kill del libro mastro nella gestione del segnale SIGALRM .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	while ((pid=waitpid(*(shared_data), &status,0))!= -1) {
			printf("Mastro %d ha deallocato ed è uscito.\n",pid);
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
		printf("Errore nel libro mastro detach nel metodo per la stampa finale.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((shmctl(shid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nella rimozione della memoria condivisa nella gestione del segnale SIGALRM.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	mem_user = shmget(MYKEY,0,SEMFLG); 
	if((shmctl(mem_user,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nella rimozione della memoria condivisa degli utenti nella gestione del segnale SIGALRM.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dal master nella gestione del segnale SIGALRM.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nella rimozione del semaforo creato dal master nella gestione del segnale SIGALRM.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_prem=semget(KEYPREM,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dall'utente.\n");
	}
	if((semctl(sem_prem,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dall'utente.\n");
	}
	printf("Simulazione finita\n");
	exit(EXIT_SUCCESS);
}
void handle_sigusr(int sig){
	int shid,*shared_data,i=0,status,sem_id,mem_user,sem_prem;
	pid_t pid;
	printf("Segnale di sigusr gestito correttamente. Terminazione dovuta al riempimento del libro mastro. Procediamo con la stampa data la terminazione della simulazione: \n");
	shid = shmget(getpid(), 0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa nella gestione del riempimento del mastro.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nella shared memory attach per gestire il segnale dovuto al riempimento del mastro.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	i=1; /*Inizialmente non invio nulla al mastro*/
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		if((kill(*(shared_data+(i)),SIGUSR2))<0){
			TEST_ERROR;
			printf("Errore nella kill del processo %d nella gestione del segnale SIGUSR.\n",*(shared_data+(i)));
			dealloca_tot();
			exit(EXIT_FAILURE);
		}
	}
	i=1;
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		while ((pid=waitpid((*(shared_data+(i))), &status,0))!= -1) {
		}
	}
	/*Stampo e controllo blocchi su mastro e prematuri*/
	stampa_finale(shared_data);
	/*Uccido il mastro dopo che ho letto quindi può deallocare e uscire*/
	if((kill(*(shared_data),SIGUSR2))<0){
		TEST_ERROR;
		printf("Errore nella kill del libro mastro nella gestione del segnale SIGUSR .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	while ((pid=waitpid(*(shared_data), &status,0))!= -1) {
			printf("Mastro %d ha deallocato ed è uscito.\n",pid);
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
		printf("Errore nella detach della memoria condivisa nella gestione del segnale SIGUSR .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((shmctl(shid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa nella gestione del segnale SIGUSR .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	mem_user = shmget(MYKEY,0,SEMFLG); 
	if((shmctl(mem_user,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa agli utenti nella gestione del segnale SIGUSR .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dal master nella gestione del segnale SIGUSR .\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nella rimozione del semaforo creato dal master nella gestione del segnale SIGUSR.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_prem=semget(KEYPREM,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dall'utente.\n");
	}
	if((semctl(sem_prem,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dall'utente.\n");
	}
	printf("Simulazione finita\n");
	exit(EXIT_SUCCESS);
}
void handle_sigill(int sig){
	int shid,*shared_data,i=0,status,sem_id,mem_user,sem_prem;
	pid_t pid;
	printf("Segnale di sigill gestito correttamente. Terminazione dovuta alla morte di tutti i processi utente. Procediamo con la stampa data la terminazione della simulazione: \n");
	shid = shmget(getpid(), 0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
		printf("Errore nel collegamento alla memoria condivisa nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
		printf("Errore nell'attach alla memoria condivisa nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	i=1; /*Inizialmente non invio nulla al mastro*/
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		if((kill(*(shared_data+(i)),SIGUSR2))<0){
			TEST_ERROR;
			printf("Errore nella kill del processo %d nella gestione del segnale SIGILL.\n",*(shared_data+(i)));
			dealloca_tot();
			exit(EXIT_FAILURE);
		}
	}
	i=1;
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		while ((pid=waitpid((*(shared_data+(i))), &status,0))!= -1) {
		}
	}
	/*Stampo e controllo blocchi su mastro e prematuri*/
	stampa_finale(shared_data);
	/*Uccido il mastro dopo che ho letto quindi può deallocare e uscire*/
	if((kill(*(shared_data),SIGUSR2))<0){
		TEST_ERROR;
		printf("Errore nella kill del mastro nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	while ((pid=waitpid(*(shared_data), &status,0))!= -1) {
			printf("Mastro %d ha deallocato ed è uscito.\n",pid);
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
		printf("Errore nella detach della memoria condivisa nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((shmctl(shid,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	mem_user = shmget(MYKEY,0,SEMFLG); 
	if((shmctl(mem_user,IPC_RMID,NULL))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione della memoria condivisa agli utenti nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dal mastro nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dal mastro nella gestione del segnale SIGILL.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
	if((sem_prem=semget(KEYPREM,0,SEMFLG))<0){
		TEST_ERROR;
		printf("Errore nella ricerca del semaforo creato dall'utente.\n");
	}
	if((semctl(sem_prem,0,IPC_RMID))<0){
		TEST_ERROR;
		printf("Errore nell'eliminazione del semaforo creato dall'utente.\n");
	}
	printf("Simulazione finita\n");
	exit(EXIT_SUCCESS);
}
void wait_for_zero(int sem_id){
	struct sembuf sops;
	sops.sem_num=0;
	sops.sem_op=0;
	sops.sem_flg=0;
	if((semop(sem_id,&sops,1))<0){
		TEST_ERROR;
		printf("Errore nella wait for zero del master sul semaforo da lui creato per la sincronizzazione.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void inizializza_sem(int sem_id){
	union semun arg;
	if((semctl(sem_id,0,SETVAL,arg.val=SO_NODES_NUM+SO_USERS_NUM+1))<0){
		TEST_ERROR;
		printf("Errore nell'inizializzazione del semaforo nel master.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void inizializza_mutex(int sem_id){
	union semun arg;
	if((semctl(sem_id,0,SETVAL,arg.val=1))<0){
		TEST_ERROR;
		printf("Errore nell'inizializzazione del semaforo mutex nel master.\n");
		dealloca_tot();
		exit(EXIT_FAILURE);
	}
}
void wait_nsecmaster(void){
	struct timespec wait_tran;
	/*
	struct timespec {
		time_t   tv_sec;        //seconds 
		long     tv_nsec;       //nanoseconds 
	};*/
	wait_tran.tv_sec=1;
	wait_tran.tv_nsec=0;
	/*La nanosleep durerà tv_sec secondi e tv_nsec nanosecondi*/
		if (nanosleep(&wait_tran, NULL) == -1) {
			switch (errno) {
			case EINTR:
			printf("nanosleep interrupted by a signal handler\n");
			case EINVAL:
			printf("tv_nsec - not in range or tv_sec is negative\n");
			default:
			perror("nanosleep");
			}
			errno=0;
		}
}
void handle_sigint(int sig){
	int shid,*shared_data,i=0,sem_id,mem_user,lmid,n_id,sem,sem_prem;
	printf("Terminazione dovuta al segnale SIGINT. Procediamo con la deallocazione e l'uscita: \n");
	shid = shmget(getpid(), 0,SEMFLG);
	if(shid==-1){
		TEST_ERROR;
	}
	shared_data=(int*)shmat(shid, NULL, 0);
	if(shared_data==(int*)-1){
		TEST_ERROR;
	}
	lmid = shmget(*(shared_data), 0,SEMFLG);
	if(lmid==-1){
		TEST_ERROR;
	}
	for(;i<SO_USERS_NUM+SO_NODES_NUM+1;i++){
		if ((n_id=msgget(*(shared_data+(SO_USERS_NUM+1+i)),0))<0) {
			TEST_ERROR;
		}
		if ((msgctl(n_id,IPC_RMID,NULL))<0) {
			TEST_ERROR;
 		}
		
	}
	i=0;
	for(;i<(SO_USERS_NUM+SO_NODES_NUM+1);i++){
		if((kill(*(shared_data+(i)),SIGKILL))<0){
		}
	}
	if((sem=semget(*(shared_data),0,SEMFLG))<0){
		TEST_ERROR;
	}
	if((semctl(sem,0,IPC_RMID))<0){
		TEST_ERROR;
	}
	if((shmdt(shared_data))<0){
		TEST_ERROR;
	}
	if((shmctl(shid,IPC_RMID,NULL))<0){
		TEST_ERROR;
	}
	if((shmctl(lmid,IPC_RMID,NULL))<0){
		TEST_ERROR;
	}
	mem_user = shmget(MYKEY,0,SEMFLG); 
	if((shmctl(mem_user,IPC_RMID,NULL))<0){
		TEST_ERROR;
	}
	if((sem_id=semget(getpid(),0,SEMFLG))<0){
		TEST_ERROR;
	}
	if((semctl(sem_id,0,IPC_RMID))<0){
		TEST_ERROR;
	}
	if((sem_prem=semget(KEYPREM,0,SEMFLG))<0){
		TEST_ERROR;
	}
	if((semctl(sem_prem,0,IPC_RMID))<0){
		TEST_ERROR;
	}
	printf("Deallocazione completata.\n");
	printf("Simulazione finita\n");
	exit(EXIT_SUCCESS);
}



