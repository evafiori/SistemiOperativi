#include <stdio.h>
#include "tree.h"

int main(){
    long x;
    treeNodePtr radice = NULL;
    char s[1];
    s[0] = 'a';
    do{
        printf("Inserisci x");
        scanf("%ld", &x);
        if( inserisciNodo(&radice, x, s) == -1){
            perror("Inserimento nodo");
        }
        s[0]++;
    }while(x != 0);
    stampaAlbero(radice);
    deallocaAlbero(&radice);
    return 0;
}