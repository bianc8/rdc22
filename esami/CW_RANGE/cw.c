/*
Esame di Reti di Calcolatori - 16 Luglio 2020

Si modifichi il web client sviluppato durante il corso per renderlo in grado di scaricare file di grandi dimensioni in
presenza di connettività di rete non affidabile caratterizzata da una frequente interruzione delle connessioni.
In queste condizioni, lo scaricamento di una risorsa di grandi dimensioni rischia di essere interrotto dalla perdita
della connessione. Risulta evidente che anche ripetendo più volte lo scaricamento del file intero si rischia ogni
volta l’interruzione e di conseguenza la probabilità di terminare con successo l’operazione diviene molto bassa.
Per ovviare a questo inconveniente il client web dev’essere modificato in modo tale che scarichi a piccoli pezzi il
file tramite una sequenza di più richieste che scaricano ciascuna un segmento del file (ad esempio di 10
Kbytes) che verranno alla fine giustapposti in un buffer per salvare il contenuto completo su un file locale.
  
Il protocollo HTTP/1.1 supporta questa funzione per mezzo dell’header “Range”. Documentato nella RFC 2616
in particolare nelle sezioni 14.35 e 14.16 e 3.12.
  
Per la sperimentazione collegarsi con il web client all’URL http://88.80.187.84:80/image.jpg

NB: Negli esperimenti non occorre simulare la perdita di connessione.


EXAMPLE REQUESTLINE
GET /immagine.jpg Host:88.80.187.84 Range:bytes=0-999
== con le variabili usate nel programma
GET /immagine.jpg Host:88.80.187.84 Range:bytes=range-range+rangeSize-1

EXAMPLE Content-Range Header in RESPONSE
Content-Range: bytes 0-1000/1674791
== con le variabili usate nel programma
Content-Range: bytes tmp1-tmp2/size

Pseudocodice:
1) itero su tanti range, apro un socket ad ogni richiesta, finche non ho scaricato la risorsa
2) dalla response, leggo l'header Content-Range, estraggo la size della risorsa da scaricare
*/ 

#include <stdio.h>
#include <string.h>  //strlen
#include <sys/types.h>  // ""
#include <sys/socket.h> // socket, struct sockaddr
#include <errno.h>
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <stdlib.h>
#include <netdb.h>  // gethostbyname

struct sockaddr_in remote;
char response[1000001]; // messa in mem statica => init a 0, char terminatore di default

struct header {
   char * n;
   char * v;
} h[100];

int main() {
    int k,n;
    char hostname[1000];
    sprintf(hostname, "88.80.187.84");

    // Resolving the hostname to an IP address
    //printf("Resolving IP for %s...\n", hostname);
    struct hostent* remoteIP = gethostbyname(hostname);
    
    char resourceName[100];
    sprintf(resourceName, "/immagine.jpg");
    FILE * fout = fopen(resourceName+1, "wb");

    char request[100];
    char * statusline;
    char hbuffer[10000];
    
    int s;
    size_t len = 0;
    int j = 0;  // scorre elementi in header
    int i = 0;  // scorre caratteri

    // RANGE
    int range = 0;
    int rangeSize = 1000;
    // Content-Range: tmp1-tmp2/size
    int tmp1,   // range
        tmp2;   // range + rangeSize - 1
    long size = 1000;   // total resource size

    // iterate ranges, must open new socket each range
    for(range=0; range < size; range += rangeSize) {
        if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0))) {
            printf("errno = %d\n", errno);
            perror("An error occured on creation of socket");
            return -1;
        }

        remote.sin_family = AF_INET;  // sa_family_t address family
        remote.sin_port = htons(80);  // in_port_t port in network byte order (16 bit)
        remote.sin_addr.s_addr = *((uint32_t *) remoteIP->h_addr_list[0]); // struct_in_addr internet address { uint32_t s_addr // address in network byte order}

        if (-1 == connect(s, (struct sockaddr *)&remote, sizeof(struct sockaddr_in))) {
            perror("Connessione socket fallita");
            return -1;
        }

        bzero(request, 100);
        sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n", resourceName, hostname, range, range+rangeSize-1);
        //printf("request: %s\n", request);

        if (-1 == write(s, request, strlen(request))) {
            perror("write fallita"); return -1;
        }
        
        bzero(hbuffer,10000);   // azzera bytes
        bzero(h,100*sizeof(struct header));

        statusline = h[0].n = hbuffer;
        for (i=0, j=0; read(s, hbuffer + i, 1); i++) {
            if (hbuffer[i] == '\n' && hbuffer[i - 1] == '\r') {
                hbuffer[i-1] = 0;      // Termino il token attuale
                if (! h[j].n[0]) break;
                h[++j].n = hbuffer + i + 1;
            }
            if (hbuffer[i] == ':' && !h[j].v) {
                hbuffer[i] = 0;
                h[j].v = hbuffer + i + 2;
            }
        }

        int bodylen=1000000;
        for(i=1; i<j; i++){
            //printf("HEADERS: %s : %s\n", h[i].n, h[i].v);
            if(!strcmp("Content-Length", h[i].n))
                bodylen=atoi(h[i].v);
            else if (!strcmp("Content-Range", h[i].n)) {
                sscanf(h[i].v+6, "%d-%d/%ld", &tmp1, &tmp2, &size);
                printf("\rDownloading %s: %.2f %%", resourceName+1, tmp2/(float)size*100);
                fflush(stdout);
            }
        }
        // read (int fd, void* buf, size_t cont)
        for (len = 0; len<bodylen && (n = read(s, response + len, bodylen - len)) > 0; len += n);
        if (n == -1) {
            perror("Read fallita"); return -1;
        }
        fwrite(response, bodylen, 1, fout);
        close(s);
    }
    fclose(fout);
    return 0;
}