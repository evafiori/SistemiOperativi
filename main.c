// #define _POSIX_C_SOURCE 200112L //necessaria intanto per fare la getopt
#include <errno.h>
#include <unistd.h>
#include <getopt.h> //anche se la getopt è definita in unistd.h
#include <sys/types.h>
#include <sys/wait.h>

#include "myLibrary.h"
#include "master.h"
#include "collector.h"
#include "tree.h"

#define HELP fprintf(stderr, "Usage: %s <fileBinario> [<fileBinario> [<fileBinario>]] [-n <nthread>] [-q <qlen>] [-d <directory-name>] [-t <delay>]\n", argv[0]);
#define NUMBER_OPTION(x, str) trad = isNumber(optarg); if(trad != -1) { if(trad > 0) { x = trad; } else { fprintf(stderr, "Argomento opzione minore o uguale a zero non valido.\n"); exit(EXIT_FAILURE); } } else { fprintf(stderr, str); exit(EXIT_FAILURE); }
#define SETTED_OPTION(c) fprintf(stderr, "Opzione %s già settata\n", c); 

extern int terminaCodaFlag;
extern int stampaFlag;

extern int socketClient;
extern int fdc;

int main(int argc, char * argv[]){
    if(argc == 1){
        HELP
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    
    int nWorker = 4, dimCoda = 8, delay = 0;
    char* dir = NULL;
    int setWorker = 0, setCoda = 0, setDelay = 0, setDir = 0;

    int c = 0; 
    int trad = 1;
    opterr = 0; //variabile impostata !=0 x default, se zero getopt non stampa messaggi di errore ma lascia fare al chiamante

    while((c = getopt(argc, argv, ":n:q:d:t:")) != -1){
        //Per impostazione predefinita, getopt () permuta il contenuto di argv durante la scansione,
        //in modo che alla fine tutte le non opzioni siano alla fine. 
        switch(c){
            case 'n': //numero dei thread worker
                if(setWorker == 0){
                    NUMBER_OPTION(nWorker, "-n: argomento non valido\n")
                    setWorker = 1;
                }
                else{
                    SETTED_OPTION("n")
                    HELP
                    exit(EXIT_FAILURE);
                }
                break;
            case 'q': //lunghezza della coda concorrente
                if(setCoda == 0){
                    NUMBER_OPTION(dimCoda, "-q: argomento non valido\n")
                    setCoda = 1;
                }
                else{
                    SETTED_OPTION("q")
                    HELP
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd': //directory contenti file binari
                if(setDir == 0){
                    dir = optarg; //controllo idDir
                    if(verificaDir(dir) == -1){
                        fprintf(stderr, "Directory specificata non valida.\n");
                        dir = NULL;
                    }
                    else{
                        setDir = 1;
                    }
                }
                else{
                    SETTED_OPTION("d")
                    HELP
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':  //delay tra richieste successive ai thread worker
                if(setDelay == 0){
                    NUMBER_OPTION(delay, "-t: argomento non valido\n")
                    setDelay = 1;
                }
                else{
                    SETTED_OPTION("t")
                    HELP
                    exit(EXIT_FAILURE);
                }
                break;
            case ':':
                fprintf(stderr, "-%c: argomento mancante.\n", (char)c);
                HELP
            case '?':
                fprintf(stderr, "-%c: opzione non valida.\n", (char)c);
                HELP

        }
        
    }
    fprintf(stderr, "Worker: %d\nCoda: %d\nDirectory: %s\nDelay: %d\n", nWorker, dimCoda, dir, delay);
    while(optind < argc){
        printf("%s\n", argv[optind++]);
    }

    //prova gestore segnali OK
    /*
        while (terminaCodaFlag != 1);
    */
    
    
    //controllare se file o directory sono stati passati come argomenti

    //prova segnali: funziona
    //while(terminaCodaFlag == 0);

    
    CHECK_EQ((pid = fork()), -1, "Fork: ")
    if(pid == 0){
        //figlio -> collector
        fprintf(stderr, "Sono il collector\n");
        ignoraSegnali();
        creaSocketServer();
        fprintf(stderr, "Fine crea socket server\n");

        //provo la socket
        int msgDim;
        char buffer[PATHNAME_MAX_DIM];
        do{
            CHECK_EQ((readn(fdc, &msgDim, sizeof(int))), -1, "readn1")
            if(msgDim > 0){
                CHECK_EQ((readn(fdc, buffer, msgDim)), -1, "readn2")
            }
            fprintf(stderr, "%d\t%s\n", msgDim, buffer);
        }while(msgDim >= 0);
        
        ATEXIT(dealloca)
    }
    else{
        //padre -> masterWoker
        fprintf(stderr, "Sono il master\n");
        //gestione segnali
        mascheraSegnali();
        creaSocketClient();
        fprintf(stderr, "Fine crea socket client\n");

        //provo la socket
        int i = 0;
        int msgDim;
        int w;
        while(terminaCodaFlag != 1 && i < argc){
            CHECK_EQ((msgDim = myStrnlen(argv[i], PATHNAME_MAX_DIM)), -1, "myStrnlen")
            msgDim++; //'\0'
            do{
                CHECK_EQ((w = writen(socketClient, &msgDim, sizeof(int))), -1, "writen")
            }while(w == 1);
            do{
                CHECK_EQ((w = writen(socketClient, argv[i], msgDim)), -1, "writen")
            }while(w == 1);
            sleep(1);
            i++;
        }
        msgDim = -1;
        do{
            CHECK_EQ((w = writen(socketClient, &msgDim, sizeof(int))), -1, "writen")
        }while(w == 1);

        //creaWorkerSet(nWorker, dimCoda);
        //metti in coda : richieste
        //tutti i file sono in coda
        // => attiva stato di uscita
        CHECK_EQ((waitpid(pid, NULL, 0)), -1, "waitpid: ")
    }

    return 0;
}