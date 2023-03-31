#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>

#define MYKEY 334455
#define KEYPREM 367890
#define SENDER -1
#define SEMFLG IPC_CREAT|0666
#define NANO 1000000000L /*La L finale indica long e serve per la conversione del timestamp in nanosecondi*/

typedef struct myuser {
		int money;
		int pid;
}user;
typedef struct mytransaction {
	long timestamp;
	pid_t sender;
	pid_t receiver;
	int money;
	int reward;
}transaction;
typedef struct {
	transaction block[SO_BLOCK_SIZE];
	int id;
} blocco;
struct mymsg {
	long mtype;             
	transaction t;
};
union semun {
    int val;
	struct semid_ds *buf;
    unsigned short  *array;
};
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}

