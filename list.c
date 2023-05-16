#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myLibrary.h"
#include "list.h"

/*
typedef struct list{
    struct list* next;
    char* file;
}l;
*/

//riceve come parametri il puntatore alla testa e il filename da inserire
//restituisce il puntatore alla nuova testa
lPtr inserisciTesta (lPtr e, char* f){
    int lung = myStrnlen(f, PATHNAME_MAX_DIM);
    if(lung == -1){
        perror("Dimensione pathname non valida: ");
        return e;
    }
    lPtr temp = malloc(sizeof(l));
    CHECK_NULL(temp, "malloc")
    CHECK_NULL((temp->file = malloc(sizeof(char)*(lung+1))), "malloc") //+1 per il terminatore di stringa
    strncpy(temp->file, f, lung);
    
    temp->next = e;
    return temp;
}

/*
//riceve come parametri il puntatore alla coda e il filename da inserire,
//restituisce il puntatore alla nuova coda
lPtr inserisciCoda (lPtr *e, char* f){
    int lung = myStrnlen(f, PATHNAME_MAX_DIM);
    if(lung == -1){
        perror("Dimensione pathname non valida: ");
        return *e;
    }
    lPtr temp = malloc(sizeof(l));
    CHECK_NULL(temp, "malloc")
    CHECK_NULL((temp->file = malloc(sizeof(char)*(lung+1))), "malloc") //+1 per il terminatore di stringa
    strncpy(temp->file, f, lung);
    temp->next = NULL;
    if(*e == NULL){
        *e = temp;
    }
    else{
        (*e)->next = temp;
    }
    return temp;
}
*/

//riceve il puntatore alla lista
//aggiorna il puntatore alla nuova testa
//libera la memoria della vecchia testa
//ritorna il filename o NULL in caso di lista vuota
char* estraiTesta (lPtr *e){
    if(*e != NULL){
        lPtr temp = *e;
        char* f = temp->file;
        *e = (*e)->next;
        free(temp);
        return f;
    }
    else{
        return NULL;
    }
}

//elimina la testa e torna il puntatore alla nuova testa
lPtr eliminaTesta (lPtr *e){
    char *s = NULL;
    if(*e != NULL){
        s = estraiTesta(e);
        free(s);
    }
    return *e;
}
/*
lPtr eliminaTesta (lPtr *e){
    if(*e != NULL){
        lPtr temp = *e;
        free(temp->file);
        *e = (*e)->next;
        free(temp);
        return *e;
    }
    else{
        return NULL;
    }
}
*/

lPtr svuotaLista(lPtr *e){
    while(*e != NULL){
        *e = eliminaTesta(e);
    }
    return *e;
}
/*
lPtr svuotaLista(lPtr *e){
    char* s = NULL;
    while(*e != NULL){
        s = estraiTesta(e);
        free(s);
    }
    return *e;
}
*/

void stampaLista(lPtr e){
    lPtr temp = e;
    while(temp != NULL){
        printf("%s\n", temp->file);
        temp = temp->next;
    }
}
