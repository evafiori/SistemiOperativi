
typedef struct list{
    struct list* next;
    char* file;
}l;

typedef l* lPtr;

lPtr inserisciTesta (lPtr e, char* f);

//lPtr inserisciCoda (lPtr *e, char* f);

char* estraiTesta (lPtr *e);

lPtr eliminaTesta (lPtr *e);

void stampaLista(lPtr e);

lPtr svuotaLista(lPtr *e);