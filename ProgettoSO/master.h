#ifndef HEADER
#define HEADER
#include "header.h"

void dealloca_tot(void);
void stampa(int * shared);
void stampa_finale(int * shared);
void handle_alarm(int sig);
void handle_sigusr(int sig);
void handle_sigint(int sig);
void handle_sigill(int sig);
void wait_for_zero(int sem_id);
void inizializza_sem(int sem_id);
void inizializza_mutex(int sem_id);
void wait_nsecmaster(void);
#endif
