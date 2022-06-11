/*
Esame 2015-7-24
Cache

Consegna:
Si modifichi il Client in modo che incorpori un meccanismo di caching delle risorse scaricate, facendo riferimento ai seguenti punti:

1) si utilizzi l'header Last-Modified dell'HTTP/1.0 documentato alla sezione 10.10 della RFC 1945

2) Ad ogni risorsa scaricata si associ un file:
    a) il cui nome corrisponde all'URI della risorsa (nel quale il carattere '/' viene sostuito dal carattere '_')
    b) il cui contenuto è composta da
        i) una prima riga contente la data di download della risorsa (espressa nel modo più conveniente)
        ii) il contenuto della risorsa (entity body)
    c) la cui cartella di salvataggio è ./cache/, figlia del working directory del programma wc.

3) per la gestione della data
    a) si faccia riferimento al formato http-date (cfr. RFC 1945 Sezione 3.3)
    b) si utilizzino le funzioni, documentate nel manuale UNIX nelle apposite sezioni riportate tra parentesi
        i) time(2) per ottenere la data espressa ini secondi a partire dal 1/1/1970 (epoch) nel tipo int rinominato time_t
        ii) localtime(3) per scomporre la data espressa in "epoch" nelle sue componenti (ora, minuti, ...) riportate ciascuna in un campo della struttura struct tm e viceversa
             mktime(3) per effettuare l'operazione inversa
        iii) opzionalmente utilizzare strftime(3) per formattare (analogamente alla printf) le componenti della data presenti nei campi della struct tm in una stringa e 
            strptime(3) per effettuare (similmente alla scanf) l'operazione inversa
    
Pseudocodice:
1) HEAD www.example.com
2) If resource is already present in dir ./cache/
    2.a) if date in Last-Modified header is <= than cached file timestamp, the cache is valid

3) downlaod the resource and save it in dir ./cache/    
    3.a) Download resource
    3.b) Save resource in dir ./cache
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>      // struct sockaddr_in
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>          // gethostbyname
#include <time.h>           // time(2) return the number of seconds since the Epoch
                            // localtime(3) broke down eoicg data in its components

#include <sys/stat.h>       // fscanf

#define _XOPEN_SOURCE       // strptime(3)

struct sockaddr_in remote;

char request[1024];
char response[1024*1024];

// header struct
struct Header {
  char *n;
  char *v;
};

// array of headers
struct Header headers[100];

// And a buffer in which the headers part of the response is store.
char hResponse[1024*1024];

char hostname[1000];


/**
* Replace char '/' with '_' in the given string
*/
void charReplace(char* s) {
    for (int i=0; s[i]; i++) {
        if (s[i] == '/')
            s[i] = '_';
    }
}

int main() {
    int s;

    if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0 ))) {
        printf("errno = %d\n",errno); perror("Socket Fallita"); return -1;
    }

    // Specify the hostname that the client needs to connect to
    sprintf(hostname, "www.example.com");

    // Specify the resource that need to be downloaded
    char resourceName[] = "/";

    // Replace '/' with '_' in resourceName
    char tmp[100];
    strcpy(tmp, resourceName);
    charReplace(tmp);

    // file to be saved in cache has name cacheName
    char cacheName[1000];
    strcpy(cacheName, "./cache/");
    strcat(cacheName, tmp);

    // Resolving the hostname to an IP address
    struct hostent* remoteIP;
    printf("Resolving IP for %s...\n", hostname);
    remoteIP = gethostbyname(hostname);
    
    
    /// 1) request HEAD for the resource
    sprintf(request, "HEAD %s HTTP/1.0\r\nHost:%s\r\n\r\n", resourceName, hostname);

    // Assign the required information to the remote address object (?)
    remote.sin_family = AF_INET;
    remote.sin_port = htons(80);
    remote.sin_addr.s_addr = *(unsigned int*)(remoteIP->h_addr_list[0]);

    // Open a connection on the socket created previously
    if (-1 == connect(s, (struct sockaddr *) &remote, sizeof(struct sockaddr_in))) {
        perror("Connect Fallita"); return -1;
    }
    /// 2) If resource is already present in dir ./cache/
    FILE* cached;
    if ((cached = fopen(cacheName, "r")) != NULL) { 
        printf("----->Resource already downloaded, verify cache\n");
        
        write(s, request, strlen(request));
        int headersCount = 0;
        headers[0].n = hResponse;
        for (int i=0; read(s, hResponse + i, 1); i++) {
            if (hResponse[i] == '\n' && hResponse[i-1] == '\r') {
                hResponse[i-1] = 0;         // Terminate token
                // If the previous header's key name is all 0s, we reached the end of the headers section.
                if (!headers[headersCount].n[0]) break;
                // The key is the string beginning just after the \n char
                headers[++headersCount].n = hResponse + i + 1;
            }
            if (hResponse[i] == ':' && !headers[headersCount].v) {
                hResponse[i] = 0;       // terminate the key
                headers[headersCount].v = hResponse + i + 1;    // value of the header
            }
        }

        // read the Last-Modified header
        int bodyLength = 0;
        char* lastModified = 0;
        for (int i=1; i<headersCount; i++)
            if (!strcmp("Last-Modified", headers[i].n))
                lastModified = headers[i].v;

        printf("Last-Modified date%s\n", lastModified);

        // HTTP DATE
        // Thu, 17 Oct 2019 07:18:26 GMT
        char* format = " %a, %d %b %Y %T GMT";     // man 3 strptime
        struct tm* httpTime = malloc(sizeof(struct tm));
        time_t epochRemote = mktime(httpTime);
        
        
        // read first line of cached file, contains timestamp of download date
        char* first_line = malloc(sizeof(struct stat));
        fscanf(cached, "%[^\n] ", first_line);
        first_line = (char*)strtoul(first_line, NULL, 0);   // remove string terminator
        time_t epochSaved = (time_t)first_line;

        printf("epoch remote %lu\n", (unsigned long)epochRemote);
        printf("epoch saved %lu\n", (unsigned long)epochSaved);

        /// 2.a) if date in Last-Modified header is <= than cached file timestamp, the cache is valid
        if (epochRemote <= epochSaved) {
            printf("----->Serve from cache!!!\n");
            int c=0;
            
            // show to the user the cached file
            char* fileBuff[1024*5];
            for (int l=0; (c = fread(fileBuff, sizeof(char), 1024*5, cached)) > 0; l += c)
                printf("%s\n", fileBuff);
            return 0;
        }
    } 

    /// 3) downlaod the resource and save it in dir ./cache/    
    printf("----->Resource need to be downloaded\n");
    char* lastModified = 0;
    
    /// 3.a) download resource
    sprintf(request, "GET %s HTTP/1.0\r\nHost:%s\r\n\r\n", resourceName, hostname);
    write(s, request, strlen(request));
    int headersCount = 0;
    headers[0].n = hResponse;
    for (int i=0; read(s, hResponse + i, 1); i++) {
        if (hResponse[i] == '\n' && hResponse[i-1] == '\r') {
            hResponse[i-1] = 0;         // Terminate token
            // If the previous header's key name is all 0s, we reached the end of the headers section.
            if (!headers[headersCount].n[0]) break;
            // The key is the string beginning just after the \n char
            headers[++headersCount].n = hResponse + i + 1;
        }
        if (hResponse[i] == ':' && !headers[headersCount].v) {
            hResponse[i] = 0;       // terminate the key
            headers[headersCount].v = hResponse + i + 1;    // value of the header
        }
    }
    // read the Content-Length, if available, else trasmission is chunked
    int bodyLength = 0;
    for (int i=1; i<headersCount; i++) {
        printf("%s: %s\n", headers[i].n, headers[i].v);
        if (!strcmp("Content-Length", headers[i].n))
            bodyLength = atoi(headers[i].v);
        
        if (!strcmp("Transfer-Encoding", headers[i].n) && !strcmp(" chunked", headers[i].v))
            bodyLength = -1;

        // caching
        if (!strcmp("Last-Modified", headers[i].n))
            // write to file cached the first line with 
            lastModified = headers[i].v;
    }
    // Chunked transfer mode
    if (bodyLength == -1) {
        // Create a buffer where we put the chunk length chars for parsing
        char chunkBuffer[8];

        // Until we have read the whole response, keep processing new chunks
        int chunkLength = 1;
        int readChunkOffset = 0;
        while(chunkLength) {
            chunkLength = 0;
            // Convert the chunkSize as hex into an int
            for (int j=0; read(s, chunkBuffer + j, 1) && !(chunkBuffer[j] == '\n' && chunkBuffer[j-1] == '\r'); j++) {
                if (chunkBuffer[j] >= 'A' && chunkBuffer[j] <= 'F')
                    chunkLength = chunkLength * 16 + (chunkBuffer[j] - 'A' + 10);
                if (chunkBuffer[j] >= 'a' && chunkBuffer[j] <= 'f')
                    chunkLength = chunkLength * 16 + (chunkBuffer[j] - 'a' + 10);
                if (chunkBuffer[j] >= '0' && chunkBuffer[j] <= '9')
                    chunkLength = chunkLength * 16 + (chunkBuffer[j] - '0');
            }
            printf("Chunk length: %d\n", chunkLength);

            // read the chunk with size of chunkLength
            int t=0, n=0;
            for (t=0, n=0; t < chunkLength && (n = read(s, response + readChunkOffset, chunkLength - t)) > 0; t += n, readChunkOffset += n);
            if (n == -1) {
                perror("Read fallita"); return -1;
            }
            // Ensure that the chunk ends with a CRLF
            for (int i=0, n=0; i < 2 && (n = read(s, chunkBuffer + i, 2)); i += n);
            if (n == -1) {
                perror("Read fallita"); return -1;
            }
            if (chunkBuffer[0] != '\r' || chunkBuffer[1] != '\n') {
                perror("Errore nel chunk"); return -1;
            }
        }
        // Terminating the response body with a string terminator
        response[readChunkOffset] = 0;
    }
    // Body-length method
    else if (bodyLength > 0) {
        int i=0, n=0;
        for (; i < bodyLength && (n = read(s, response + i, 2)) > 0; i += n);
        if (n == -1) {
            perror("Read fallita"); return -1;
        }
        response[i] = 0;        // Terminating response
    }

    /// 3.b) Save resource in dir ./cache
    cached = fopen(cacheName, "w+");
    
    // save the timestamp in the cached file
    time_t epochNow = time(NULL);
    fprintf(cached, "%lu\n", (unsigned long)epochNow);

    // save the entity body in the cached file
    fwrite(response, sizeof(response[0]), strlen(response), cached);

    // show to the user the response
    printf("%s", response);

    fclose(cached);
    return 0;
}
