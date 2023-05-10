#define _POSIX_C_SOURCE  200112L

#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "myLibrary.h"
#include "master.h"

#define SOCKNAME "./farm.sck"

int terminaCodaFlag = 0;
int stampaFlag = 0;

pthread_t idGestoreSegnali = -1;
int socketClient;

pthread_mutex_t mtxSocket = PTHREAD_MUTEX_INITIALIZER;

//fa terminare il thread gestore e libera la memoria tramite la join
void killGestoreSegnali() {
    fprintf(stderr, "id gestore segnali: %ld\n", idGestoreSegnali);
    CHECK_NEQ((errno = pthread_kill(idGestoreSegnali, SIGTERM)), 0, "pthread_kill") //SIGTERM è gestito: aziona terminaCoda e fa exit()
    fprintf(stderr, "La kill e' stata eseguita\n");
    CHECK_NEQ((errno = pthread_join(idGestoreSegnali, NULL)), 0, "join gestore segnali") 
    fprintf(stderr, "La join è stata eseguita\n");
}

//thread che cattura tutti i segnali e li gestisce adeguatamente
void* gestoreSegnali(void* mask){
    int sig;
    
   fprintf(stderr, "gestore star\n");
    CHECK_NEQ((errno = pthread_sigmask(SIG_UNBLOCK, (sigset_t *) &mask, NULL)), 0, "pthread_sigmask: ")
    
    while(1){ 
        
        CHECK_NEQ((errno = sigwait(&mask, &sig)), 0, "sigwait: ")
        fprintf(stderr, "sblocato\n");
        switch(sig){
            case SIGHUP:
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                terminaCodaFlag = 1;
                fprintf(stderr, "gestore\n");
                pthread_exit(NULL);
            case SIGUSR1:
                stampaFlag = 1;
                //prendere direttamente la lock sulla socket e mandare messaggio di stampa
                break;
            // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
            //quindi SIGPIPE non fa parte della maschera
            
        }
    }
    //pthread_exit(NULL); //dovrò killarlo in uscita, serve gestire continuamente i segnali
    //il ciclo è solo per poter ricevere quante volte mi pare SIGUSR1
    //non è un problema se il gestore termina perché agli altri thread i segnali sono mascherati
    //esce con una kill: ricezione segnale SIGTERM
}

//maschera i segnali a tutti i thread e chiama un thread gestore apposta
void mascheraSegnali(){
    sigset_t mask;
    sigset_t old;
    struct sigaction s;

    //aggiungo i segnali alla maschera
    CHECK_EQ(sigemptyset(&mask), -1, "sigemptyset: ") 
    CHECK_EQ(sigaddset(&mask, SIGHUP), -1, "SIGHUP")
    CHECK_EQ(sigaddset(&mask, SIGINT), -1, "SIGINT")
    CHECK_EQ(sigaddset(&mask, SIGQUIT), -1, "SIGQUIT")
    CHECK_EQ(sigaddset(&mask, SIGTERM), -1, "SIGTERM")
    CHECK_EQ(sigaddset(&mask, SIGUSR1), -1, "SIGUSR1")
    //potrei mascherare anche SIGPIPE
    //LA MASCHERA È EREDITATA DOPO LA FORK
    //CHECK_EQ(sigaddset(&mask, SIGPIPE), -1, "SIGPIPE") //maschero anche la pipe o SIG_IGN?

    //blocco segnali della maschera a tutti gli altri thread
    CHECK_NEQ((errno = pthread_sigmask(SIG_BLOCK, &mask, &old)), 0, "pthread_sigmask: ")
    //int sig;
    //CHECK_NEQ((errno = sigwait(&mask, &sig)), 0, "sigwait: ")
    //fprintf(stdout, "sblockk\n");  
    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
    //memset(&s, 0, sizeof(s)); 
    //s.sa_handler = SIG_IGN;
    //CHECK_EQ(sigaction(SIGPIPE, &s, NULL), -1, "sigaction")

    //creo thread gestore dei segnali
    CHECK_NEQ((errno = pthread_create(&idGestoreSegnali, NULL, gestoreSegnali, NULL)), 0, "Thread create: ")
    //IL THREAD È CREATO QUANDO LO DECIDE IL SO
    
    //ATEXIT(killGestoreSegnali)
}


void chiudiSocketClient(){
    LOCK(mtxSocket)
    CLOSE(socketClient, "Close socket client: ")
    UNLOCK(mtxSocket)
}

//crea la connessione socket lato client
void creaSocketClient(){
    struct sockaddr_un sa;
    
    CHECK_EQ((socketClient = socket(AF_UNIX, SOCK_STREAM, 0)), -1, "Socket: ")

    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    CHECK_EQ(myConnect(socketClient, (struct sockaddr *) &sa, sizeof(sa)), -1, "Connessione fallita: ")
    ATEXIT(chiudiSocketClient)
}