#define _POSIX_C_SOURCE  200112L

#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
 #include <unistd.h>

#include "tree.h"
#include "myLibrary.h"

#define SOCKNAME "./farm.sck"

//globali per essere utilizzati nelle funzioni atexit
int socketServer;
int fdc;

//ignora tutti i segnali
void ignoraSegnali(){
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
    CHECK_EQ(sigaction(SIGHUP,&sa, NULL), -1, "Sigaction SIGHUP: ")
    CHECK_EQ(sigaction(SIGINT,&sa, NULL), -1, "Sigaction SIGINT: ")
    CHECK_EQ(sigaction(SIGQUIT,&sa, NULL), -1, "Sigaction SIGQUIT: ")
    CHECK_EQ(sigaction(SIGTERM,&sa, NULL), -1, "Sigaction SIGTERM: ")
    CHECK_EQ(sigaction(SIGPIPE,&sa, NULL), -1, "Sigaction SIGPIPE: ")
    CHECK_EQ(sigaction(SIGUSR1,&sa, NULL), -1, "Sigaction SIGUSR1: ")
}

//chiude collegamento tramite filedescriptor
void chiudiSocketServer(){
    CLOSE(socketServer, "Close socket server: ")
}

//cancella il file farm.sck creato
void cancellaFileSocket(){
    unlink(SOCKNAME);
}

//chiude connessione tramite filedescriptor
void chiudiConnessione(){
    CLOSE(fdc, "Close socket connection: ")
}

//si occupa della socket dalla creazione alla connessione, 
//aggiungendo in exit le funzioni necessarie alla chiusura dei fd e cancellazione del file
void creaSocketServer(){
    struct sockaddr_un sa;

    CHECK_EQ((socketServer = socket(AF_UNIX, SOCK_STREAM, 0)), -1, "socket")
    ATEXIT(chiudiSocketServer)

    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    CHECK_EQ(bind(socketServer, (struct sockaddr *) &sa, sizeof(sa)), -1, "Bind: ")
    ATEXIT(cancellaFileSocket)

    CHECK_EQ(listen(socketServer, 1), -1, "Listen: ")
    //accetto una sola connessione, uso la accept (che può essere interrota da EINVAL)
    
    CHECK_EQ((fdc = accept(socketServer, NULL, 0)), -1, "Accept: ")
    ATEXIT(chiudiConnessione)
}

/*
int main(int argc, char* argv[]){
    ignoraSegnali();

    creaSocketServer();

    ATEXIT(dealloca)
    
    msg_t* messaggio = NULL;
    int pathDim = 0;
    char* buffer = NULL;
    
    //riceve messaggi

    //while(nameDim > 0)

    stampaAlbero();
    //dealloca già tramite atexit

    return 0;
}
*/