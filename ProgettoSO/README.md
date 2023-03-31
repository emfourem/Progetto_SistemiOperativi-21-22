RELAZIONE VERSIONE MINIMA
-Studente : Merico Michele

MASTER
La simulazione parte dal processo master che esegue i seguenti compiti:
1)Creazione della memoria condivisa (1.1) per tutti i processi in cui verranno inserite le informazioni relative a quanti sono i blocchi nel libro mastro, quanti sono i processi utenti terminati prematuramente e tutti i pid dei nodi, degli utenti e del libro mastro;
1.1) Segmento di interi dato che “The type of pid_t data is a signed integer type or we can say int” e non conterrà solo pid dei processi.
2)Creazione della memoria condivisa (2.1) per i processi utente affinchè possano: generare transazioni sapendo, ad ogni invio, scegliere in maniera corretta il receiver della transazione in maniera tale da non estrarre sè stesso o un processo utente precedentemente terminato e tener conto invio dopo invio di quanto denaro hanno inviato effettivamente;
2.1) Segmento formato da esattamente SO_USERS_NUM strutture predefinite chiamate user e ogni struttura user è così definita:
typedef struct myuser {
		int money;
		int pid;
}user;
3)Creazione di un semaforo inizializzato al numero totale dei processi, master escluso, che servirà per sincronizzare tutti i processi e far sì che partano contemporaneamente; creazione di un semaforo di mutua esclusione per gli utenti che dovranno scrivere in memoria condivisa che sono terminati prematuramente.
Una volta create e inizializzate le variabili accessorie oltre che tutto quello elencato nei punti 1,2,3, il master crea (fork()) e esegue(execvp()) tutti i SO_USERS_NUM,i SO_NODES_NUM e il processo mastro.
Terminata la creazione dei figli esegue una wait for zero sul semaforo creato in precedenza e quando questo diventa zero inizia la simulazione e il master entra in un loop infinito in cui ad ogni secondo esegue una stampa per tener traccia di tutto quello che sta succedendo finchè non occorre qualcosa che farà terminare la simulazione.
 All'interno del loop infinito ci sarà una stampa dello stato attuale della simulazione e quindi del budget di tutti i processi ad intervalli regoalari di un secondo. La regolarità degli intervalli è gestita da una nanosleep della durata di un secondo e la scelta della nanosleep è dovuta al fatto che la documentazione trovata in rete ha messo in luce come la 'collaborazione' alarm e sleep avrebbe potuto creare dei problemi e in particolare “sleep() puo' essere basata sull'arrivo del segnale SIGALRM, pertanto e' da evitare un uso contemporaneo di sleep() e della system call alarm()“.
Il master si avvale di vari metodi ausiliari fra i quali i più importanti sono: il metodo per il calcolo del budget di tutti i processi (nodo e utenti); un metodo utile in caso di deallocazione forzata dovuta ad un errore nell'esecuzione; tre metodi in risposta a degli eventi che potrebbero far terminare la simulazione. Gli eventi che potrebbero causare ciò sono:
1)Lo scatto dell'allarme ovvero quando son passati esattamente SO_SIM_SEC dall'inizio della simulazione che viene gestito da un handler predefinito chiamato handle_alarm;
2)Il riempimento del libro mastro che obbliga il processo mastro ad inviare un segnale di SIGUSR1 al processo master che in risposta gestisce il segnale con l'handler definito appositamente chiamato handle_sigusr;
3)La terminazione di tutti i processi utente in maniera prematura che porta così il penultimo utente, che sta per terminare, ad inviare un segnale di SIGILL al mastro che gestendo tale segnale con l'handler chiamato handle_sigill forza anche l'ultimo processo ancora attivo a terminare e imposta i processi prematuri a SO_USERS_NUM.
Ciò che accomuna i tre handler è il fatto che in ognuno di essi:
1) viene stampata una ricapitolazione generale della situazione dei processi nel momento in cui termina la simulazione;
2) vengono deallocate tutte le risorse IPC che il processo master ha creato lasciando la deallocazione delle risorse utili privatamente ai singoli processi ai processi stessi dopo la ricezione del segnale SIGUSR2 che egli stesso provvede ad inviare.

UTENTE 
Ogni processo utente inizia sempre con i collegamenti alle memorie condivise create dal master, poi inizializza le variabili necessarie all'esecuzione e dopo aver decrementato il semaforo creato dal master si mette in attesa (wait for zero) sul semaforo. Quando diventa zero  inizia la vera e propria esecuzione dell'user. Iterativamente il processo tramite il metodo calc_bilancio verifica di avere un budget sufficiente a inviare una transazione e, se lo è, l'utente estrae randomicamente un numero compreso fra 0 e SO_NODES_NUM che servirà da indice all'interno del segmento di memoria condivisa per accedere al pid del nodo desiderato. Una volta trovato il pid del nodo destinatario,l'utente si collega alla coda di messaggi privata del nodo che avrà come chiave il pid stesso e una volta ottenuto il collegamento, l'utente genera una transazione, una struttura predefinita con cinque campi:
typedef struct mytransaction {
	long timestamp;
	pid_t sender;
	pid_t receiver;
	int money;
	int reward;
}transaction;
1)timestamp->esso viene impostato prendendo il valore di ritorno della funzione clock_gettime usando come orologio il CLOCK MONOTONIC ovvero un orologio che non può essere impostato e rappresenta il tempo monotono da un punto di partenza non specificato che viene trasformato in nanosecondi tramite la moltiplicazione del valore tv_sec e la costante NANO a cui viene sommata la componente tv_nsec;
2)sender->implicito essendo l'utente stesso ad inviare;
3)receiver->un altro utente estratto randomicamente sfruttando la funzione estrai_rand definita appositamente che estrae un valore che verrà usato come indice all'interno della memoria condivisa degli utenti;
4)money->ovvero un valore randomico estratto con estrai_rand fra 2 e il bilancio attuale dell'utente a cui viene sottratto il reward;
5)reward->viene calcolato con la seguente formula:SO_REWARD*il valore randomico estratto nel punto precedente e poi il tutto fratto 100 così da ottenere il SO_REWARD% del valore estratto.
Per ogni transazione inviata, l'utente aggiorna nella memoria condivisa degli utenti e, in particolare nel campo money della struttura user, il valore di tale campo così da sapere ad ogni richiamo della funzione calc_bilancio quanto è il denaro che effettivamente ha inviato.
L'utente gestisce anche tre segnali che potrebbero arrivargli tramite sigaction:
1)SIGUSR1->tale segnale viene inviato all'utente dal nodo quando quest'ultimo avendo la transaction pool piena è costretto a scartare, dopo aver riaccreditato il denaro al sender, una o più transazioni in cui l'utente è sender;
2)SIGUSR2->tale segnale viene inviato dal processo master quando la simulazione è finita e forza il processo utente a terminare la propria esecuzione;
3)SIGQUIT->tale segnale invece viene gestito e la sua ricezione causa la creazione e l'invio di una transazione se l’utente ha un budget sufficiente(>=2).
Ogni utente ogni qualvolta non riesce ad inviare una transazione aggiorna il valore della variabile tentativi e quando questo diventa pari a SO_RETRY, l’utente aggiorna il numero dei processi prematuri in memoria condivisa, mette il proprio pid a 0 nella memoria condivisa degli utenti(campo pid della struttura user) e controlla che non sia il PENULTIMO processo ancora attivo. Se lo è significa che tutti i processi sono terminati prematuramente e l’ultimo rimasto non potrà più generare alcuna transazione, perciò invia un segnale di SIGILL al master che forza la terminazione dell’ultimo processo ancora attivo e obbliga tutti i processi alla deallocazione delle risorse allocate nell’esecuzione. Se non lo è aggiorna semplicemente il numero dei processi prematuri e termina. L’accesso a tale campo della memoria condivisa è gestito da un semaforo di mutua esclusione creato dal master.

NODO
I processi nodo esattamente come i processi utente iniziano collegandosi alla memoria condivisa dei processi e alla memoria condivisa degli utenti in ottica futura data la possibilità di dover scartare transazioni e quindi di dover riaccreditare denaro ad un dato utente. Inoltre dopo aver inizializzato e definito tutte le variabili accessorie i processi nodo trovano, decrementano e si mettono in attesa(wait for zero) sul semaforo del master ma solo dopo aver creato la propria coda privata che sarà a tutti gli effetti la transaction pool del nodo. 
Non appena il semaforo diventa zero i nodi cercano l’identificatore del semaforo creato dal processo mastro e anche il collegamento al libro mastro vero e proprio; tutto ciò sarà utile per per scrivere correttamente i blocchi sul libro mastro essendo i nodi gli addetti a tale operazione.
Queste ultime due operazioni di ‘ricerca’ vengono fatte dopo la wait for zero dato che non si sa precisamente (interleaving) quando il libro mastro creerà le risorse, ma lo farà sicuramente prima di decrementare il semaforo del master e quindi prima di mettersi in attesa dello zero anch’esso. 
Una volta compiute tutte queste azioni il nodo inizia il suo loop in cui attende che ci siano o almeno SO_BLOCK_SIZE - 1 transazioni sulla coda di messaggi(trattata come una coda first in first out) o non ce ne siano un numero maggiore di SO_TP_SIZE e in base a questo si comporta di conseguenza:
1)Se ci sono almeno SO_BLOCK_SIZE – 1 transazioni, ne riceve SO_BLOCK_SIZE - 1, ci aggiunge la transazione predefinita in ultima posizione e le memorizza in una struttura predefinita chiamata blocco. 
Tale struttura è così formata:
typedef struct {
	transaction block[SO_BLOCK_SIZE];
	int id;
} blocco;
Una volta creato il blocco,accede in mutua esclusione al libro mastro, quindi fa una reserve sul semaforo, aggiorna l’id del blocco in base al valore contenuto nella memoria condivisa creata dal master, ne simula l’elaborazione con una nanosleep, scrive tale blocco sul libro mastro, controlla che non si sia raggiunta la massima capienza del libro mastro, rilascia il semaforo e riprende il ciclo a partire dal controllo sul numero di transazioni in coda.
2)Se ci sono più di SO_TP_SIZE transazioni nella coda, il nodo si salva le SO_TP_SIZE transazioni arrivate prima in un array e scarta tutte le restanti finché il numero di transazioni in coda non ritorna ad essere pari a 0. Poi vengono creati i blocchi dalle transazioni salvate in precedenza,scritti sul mastro sempre in mutua esclusione e il conteggio dei messaggi non letti(ancora da processare quindi) è trovato sommando i messaggi che restano in coda più la variabile remaining che viene mantenuta durante l’esecuzione. Una volta finito ciò il nodo riparte con lo stesso ciclo di controllo. Ad ogni scarto, il nodo avvisa il sender inviandogli un segnale di SIGUSR1 e gli restituisce il denaro che aveva inviato andando ad aggiungerlo nel campo money della struttura user relativa a tale processo utente nella memoria condivisa degli utenti.



PROCESSO MASTRO
Il processo mastro crea il libro mastro, segmento di memoria condivisa di SO_REGISTRY_SIZE blocchi , e un semaforo di mutua esclusione utile per regolare l’accesso in scrittura dei nodi.
La struttura blocco è la seguente: 
typedef struct {
	transaction block[SO_BLOCK_SIZE];
	int id;
} blocco;
Una volta create tali risorse, il processo mastro trova, decrementa e si mette in attesa sul semaforo del master.
Una volta uscito da questa wait for zero, il processo mastro esegue pause() e attende che il master gli segnali di deallocare ed uscire.


NOTE FINALI
-La definizione di TEST ERROR è stata copiata dal codice presentatoci a lezione e inclusa nel file header.h.
-La definizione o meno di SO_MIN_TRANS_PROC_NSEC e SO_MAX_TRANS_PROC_NSEC è gestita tramite controllo sulla lunghezza di argv quindi sul valore di argc. Tutti i processi inoltre ricevono le variabili a run time e le salvano in delle variabili dichiarate extern così da essere visibili a tutto il processo. 
La variabile extern pausa nei nodi serve per indicare che le costanti  SO_MIN/MAX_TRANS_PROC_NSEC sono definite; infatti, se pausa vale 1 allora saranno definite e la simulazione dell’elaborazione di un blocco sarà gestita in maniera diversa rispetto a quando non saranno definite e pausa varrà 0.
