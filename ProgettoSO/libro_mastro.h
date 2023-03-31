#ifndef HEADER
#define HEADER
#include "header.h"

void dealloca_tot(void);
void dealloca_mastro(void);
void wait_for_zero(int sem_id);
void reserve_sem(int sem_id);
int sem_find(void);
int sem_crea(void);
void inizializza_sem(int sem_id,int val);
void handle_sigusr2(int sig);
#endif