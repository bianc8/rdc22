#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int tmp;
// Struct dell'header. Subito dichiarata e inizializzata a dimensione 100.
struct header {
    char *n; //nome
    char *v; //valore
} h[100];

int main() {
    // Puntatore alla SL (Status Line)
    char *statusline;
    // Indirizzo IP
    struct sockaddr_in addr;
    // Lunghezza Entity-Body
    int entity_length;
    // Flag di controllo
    int is_chunked;
    // Contatori
    int i, j, k, s, t;
    // Buffer request e Buffer Response
    char request[5000], response[10000];
    // Puntatore all'Entity Body
    char *entity;
    // Indirizzo IP di destinazione
    unsigned char targetip[4] = { 216, 58, 213, 100 };
//unsigned char targetip[4] = { 213,92,16,101 };
    // Socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        tmp = errno;
        perror("Socket fallita");
        printf("i=%d errno=%d\n", i, tmp);
        return 1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // Non c'e' bisogno di girare i byte, in quanto si parte da un array di valori, a cui viene fatto il cast
    addr.sin_addr.s_addr = *(unsigned int*) targetip; // <indirizzo ip del server 216.58.213.100 >

    if (-1 == connect(s, (struct sockaddr*) &addr, sizeof(struct sockaddr_in)))
        perror("Connect fallita");
    // Stampa di debug del valore restituito dalla funzione socket.
    printf("%d\n", s);
    // Ciclo per effettuare N richieste. iter < 1 => 1 richiesta. iter < 1000 => 1000 richieste.
    for (int iter = 0; iter < 1; iter++) {
        // Stampa su buffer request la richiesta da fare al server
        sprintf(request, "GET / HTTP/1.1\r\nHost:www.google.com\r\n\r\n");
        // Generazione della request (scrittura nel socket), e controllo se restituisce un errore
        if (-1 == write(s, request, strlen(request))) {
            perror("write fallita");
            return 1;
        }
        // Imposta i byte a zero nell'array h (header).
        bzero(h, sizeof(struct header) * 100);
        // La SL e' la prima riga della Response, la quale viene copiata nel primo array di header.
        statusline = h[0].n = response;
        // Lettura della risposta dalla read, che trascrive nel buffer response, carattere per carattere.
        for (j = 0, k = 0; read(s, response + j, 1); j++) {
            // Se la read incontra il token ':' , e il valore di quell'header e' nullo...
            if (response[j] == ':' && (h[k].v == 0)) {
                // ...lo imposta come terminatore.
                response[j] = 0;
                // Da dopo il carattere terminatore appena impostato, legge il valore.
                h[k].v = response + j + 1;
                // Se legge un CRLF...
            } else if ((response[j] == '\n') && (response[j - 1] == '\r')) {
                // ....Dove c'e' il \n imposta il terminatore...
                response[j - 1] = 0;
                // Se il nome dell'header precedente e' nullo, vuol dire che c'e' un doppio CRLF!
                if (h[k].n[0] == 0)
                    break;
                // Se, invece, non e' nullo, allora ne acquisisce il nome
                h[++k].n = response + j + 1;
            }
        }
        entity_length = -1;
        // Stampo la SL.
        printf("Status line = %s\n", statusline);
	// Inizializzo il flag is_chunked.
	is_chunked = 0;
        for (i = 1; i < k; i++) {
            if (strcmp(h[i].n, "Content-Length") == 0) {
                entity_length = atoi(h[i].v);
                printf("* (%d) ", entity_length);
            }
	    else if(strcmp(h[i].n, "Transfer-Encoding") == 0) {
		if(strcmp(h[i].v, " chunked") == 0){
		    // Si leggono i chunk.
		    is_chunked = 1;
		}
	    }
	    
            printf("%s ----> %s\n", h[i].n, h[i].v);
        }
        // [DEBUG] imposto l'entity length ad un valore predefinito per vedere che succede con tale valore, o imposto a -1 per simulare l'assenza di Entity Body.
        entity_length = -1;
        // In assenza di Entity Body, imposto la lunghezza ad un valore elevato.
        if (entity_length == -1)
            entity_length = 1000000;
        // Imposto buffer per l'Entity Body.
        entity = (char*) malloc(entity_length);

	if(is_chunked){
	    entity_length = 0;
        int chunk_length = 0, row_end = 1, cp = 0, is_valid = 0, rl = 0, ep = 0;
        char *chunk_size = malloc(100);
	    do{
            int j = 0;
            // Leggo Entity Body, ricopiandolo nel buffer entity, fino alla fine della read.
            // Lo scopo del ciclo while e' di determinare la lunghezza di cio' che e' stato letto dalla read.
            // Lettura chunk_size fino primo CRLF.
            for(j=0; (t = read(s, (chunk_size + j), 1)) > 0;++j){
                if(( chunk_size[j] == '\n' ) && ( chunk_size[j-1] == '\r' ) ){
                    chunk_size[j-1] = 0;
                    break;
                }
            }
            int digit = 0;
            chunk_length = 0;
            // convert hex to int
            for(j=0; chunk_size[j] != 0 && chunk_size[j] != ' ';++j){
                if(chunk_size[j] >= '0' && chunk_size[j] <= '9')
                    digit = chunk_size[j] -  '0';
                else if(chunk_size[j] >= 'A' && chunk_size[j] <= 'F')
                    digit = 10 + chunk_size[j] - 'A';
                else if(chunk_size[j] >= 'a' && chunk_size[j] <= 'f')
                    digit = 10 + chunk_size[j] - 'a';
                chunk_length = chunk_length*16 + digit;
            }
            if(!chunk_length)
                break;
            entity_length = entity_length + chunk_length;
            entity = (char*) realloc(entity,entity_length);
            for(j=0; (t = read( s, entity+ep+j, chunk_length-j ) ); j+=t);
            ep += j;
            read(s, chunk_size,1);
            read(s, chunk_size+1,1);
            if( ( chunk_size[0] != '\r' ) || ( chunk_size[1] != '\n' ) ){
                printf("Errore di parsing. Mancano CR e LF.\n");
            }
	    
	    } while(chunk_length > 0);

	// Lettura della risposta dalla read, che trascrive nel buffer response, carattere per carattere.
        for (j = 0; read(s, response + j, 1); j++) {
            // Se la read incontra il token ':' , e il valore di quell'header e' nullo...
            if (response[j] == ':' && (h[k].v == 0)) {
                // ...lo imposta come terminatore.
                response[j] = 0;
                // Da dopo il carattere terminatore appena impostato, legge il valore.
                h[k].v = response + j + 1;
                // Se legge un CRLF...
            } else if ((response[j] == '\n') && (response[j - 1] == '\r')) {
                // ....Dove c'e' il \n imposta il terminatore...
                response[j - 1] = 0;
                // Se il nome dell'header precedente e' nullo, vuol dire che c'e' un doppio CRLF!
                if (h[k].n[0] == 0)
                    break;
                // Se, invece, non e' nullo, allora ne acquisisce il nome
                h[++k].n = response + j + 1;
            }
        }
        free(chunk_size);
        }
    //if ( t == -1) { perror("Read fallita"); return 1;}
    printf("j= %d\n", j);

    for (i = 0; i < entity_length; i++)
        printf("%c", entity[i]);
    
    }
    free(entity);
}