#include <pthread.h>
#include <errno.h>

#include "myLibrary.h"
#include "threadpool.h"

extern int terminaCodaFlag;

//pop: la chiamata deve avvenire in possesso della LOCK
char* pop(BQueue_t *q){
    if(q == NULL){
        errno = EINVAL;
        return NULL;
    }
    char* data = q->queue[q->head];
    q->queue[q->head] = NULL;
    q->head = (q->head+1) % q->qsize;
    q->qlen--;

    return data;
}

//push

void *worker_thread(void *threadpool) {    
    BQueue_t *pool = (BQueue_t *)threadpool; // cast
    char* path = NULL;

    LOCK(pool->m)
    while(terminaCodaFlag == 0 || pool->qlen > 0) {

        // in attesa di un messaggio o del flag di terminazione
        while(pool->qlen == 0 && terminaCodaFlag == 0) {
            WAIT(pool->cempty, pool->m)
	    }

        if (terminaCodaFlag == 1 && pool->qlen == 0) {
            // devo uscire e non ci sono task in coda
            UNLOCK(pool->m)
            return NULL;
        }
        
	    // nuovo task
        // data = pop
    
        //UNLOCK(pool->m)

        // eseguo la funzione 
        // invio il risultato
	
        //LOCK(pool->m)
       
    }
    UNLOCK(pool->m)

    return NULL;
}

//ritorna -1 con errno settato in caso di errore
//0 se tutto okay
int freePoolResources(BQueue_t *pool) {
    if(pool == NULL){
        errno = EINVAL;
	    return -1;
    }

    int returnResult = 0;

    if(pool->threads) {
        free(pool->threads);
    }

    if(pool->queue != NULL) {
        for(int i=0; i < pool->qsize; i++) {
            if(pool->queue[i] != NULL){
                free(pool->queue[i]);
            } 
        }
        free(pool->queue);
    }
	
    if((errno = pthread_mutex_destroy(&(pool->m))) != 0){
        perror("destroy mutex");
        returnResult = -1;
    }
    if((errno = pthread_cond_destroy(&(pool->cfull))) != 0){
        perror("destroy cond cfull");
        returnResult = -1;
    }
    if((errno = pthread_cond_destroy(&(pool->cempty))) != 0){
        perror("destroy cond cempty");
        returnResult = -1;
    }

    free(pool);    
    if(returnResult == -1){
        errno = EINVAL;
        return -1;
    }
    return 0;
}

// fa terminare i thread e li joina
// la chiamo quando 
    // ho un fallimento durante il lancio dei thread
    // arriva un segnale di terminazione
    // finisco i messaggi da mettere in coda 
//ritorna -1 in caso di errore con errno settato
int destroyThreadPool(BQueue_t *pool) {    
    if(pool == NULL) {
        errno = EINVAL;
        return -1;
    }

    //la lock mi serve solo per accedere alla condition variable
    LOCK(pool->m)

    //cempty è la cond sui cui mi restano in attesa i worker (thread)
    if (pthread_cond_broadcast(&(pool->cempty)) != 0) { 
      UNLOCK(pool->m);
      errno = EFAULT;
      return -1;
    }
    UNLOCK(pool->m)

    for(int i = 0; i < pool->numthreads; i++) {
	if (pthread_join(pool->threads[i], NULL) != 0) {
	    errno = EFAULT;
	    UNLOCK(pool->m)
	    return -1;
	}
    }
    //freePoolResources(pool); la devo chiamare a prescindere, non necessariamente qua
    return 0;
}

BQueue_t *createThreadPool(int num, int size) {
    //questo controllo io lo faccio già quando leggo gli argomenti
    if(num <= 0 || size < 0) {
	    errno = EINVAL;
        return NULL;
    }
    
    BQueue_t *pool;
    CHECK_NULL((pool = (BQueue_t *) malloc(sizeof(BQueue_t))), "malloc pool")
    
    //inizializzazione
    pool->numthreads   = 0;
    pool->taskonthefly = 0;
    pool->qsize = size;
    pool->qlen = 0;
    pool->head = 0;
    pool->tail = 0;

    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num);
    if (pool->threads == NULL) {
        //error: ENOMEM
        free(pool);
        return NULL;
    }
    pool->queue = (char**) malloc(sizeof(char*) * size);
    if (pool->queue == NULL) {
        //error: ENOMEM
        free(pool->threads);
        free(pool);
        return NULL;
    }
    else{
        for(int i = 0; i < pool->qsize; i++){
            pool->queue[i] = NULL;
        }
    }
    
    if ((errno = pthread_mutex_init(&(pool->m), NULL)) != 0){
        free(pool->threads);
        free(pool->queue);
        free(pool);
        if((errno = pthread_mutex_destroy(&(pool->m))) != 0){
            perror("destroy mutex");
        }
        return NULL;
    }
    if((errno = pthread_cond_init(&(pool->cfull), NULL) != 0)){
        free(pool->threads);
        free(pool->queue);
        free(pool);
        if((errno = pthread_cond_destroy(&(pool->cfull))) != 0){
            perror("destroy cond cfull");
        }
        return NULL;
    } 
    if((errno = pthread_cond_init(&(pool->cempty), NULL)) != 0){
        free(pool->threads);
        free(pool->queue);
        free(pool);
        if((errno = pthread_cond_destroy(&(pool->cempty))) != 0){
            perror("destroy cond cempty");
        }
        return NULL;
    }
    for(int i = 0; i < num; i++) {
        if((errno = pthread_create(&(pool->threads[i]), NULL, worker_thread, (void*)pool)) != 0) {
	    /* errore fatale, libero tutto forzando l'uscita dei threads */
        //la forzo mettendo il flag a più di uno
            terminaCodaFlag = 1;
            destroyThreadPool(pool);
	        errno = EFAULT;
            return NULL;
        }
        pool->numthreads++;
    }
    return pool;
}
//dopo questo^ definire correttamente destroy... MA NON POTREI VOLER LAVORARE CON I THREAD CHE SONO RIUSCITA A CREARE??

