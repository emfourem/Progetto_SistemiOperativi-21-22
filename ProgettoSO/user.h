#ifndef HEADER
#define HEADER
#include "header.h"

int find_index(int* shared);
int estrai_random(int estremo_sx,int estremo_dx);
void wait_nsec(void);
void wait_for_zero(int sem_id);
int sem_find(int chiave);
int find_sem(int pid);
void release_sem(int sem_id);
void reserve_sem(int sem_id);
int calc_bilancio(int money,int pid_mastro);
transaction genera(int bilancio,int receiver);
int coda_find(int destinatario);
void handle_sigquit(int sig);
void handle_sigusr2(int sig);
void handle_sigusr1(int sig);
#endif