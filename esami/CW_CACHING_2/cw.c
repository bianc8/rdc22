/*
Università degli Studi di Padova - Dipartimento di Ingegneria Informatica
Esame di Reti di Calcolatori - 30 Agosto 2018

Si modifichi il programma web client wc18.c in modo tale che sia in grado di gestire il meccanismo di controllo
del caching basato sugli header Last-Modified e If-Modified-Since descritti nella RFC2616 ai capitoli 14.29,
14.25.

Il programma wc18.c modificato, per la verifica del corretto funzionamento, dovrà essere azionato due volte.
La prima volta, a cache vuota, dovrà acquisire la data di ultima modifica della risorsa e la salverà insieme al entity
body sulla cache (file su disco).
La seconda volta (e le successive) ri-scaricherà la risorsa solo se è questa è stata modificata nel frattempo sul
server, altrimenti dovrà accedere alla copia in cache.

Pseudocodice:
1.a) If resource is already present in dir ./cache/
    GET www.example.com If-Modified-Since:httpDate(timestamp)
    --> Response
    HTTP/1.1 304 Not Modified --> return cached
1.b) Else resource not present
    GET www.example.com
--> Response
Else) HTTP/1.1 200 OK --> Download the resource and save it in dir ./cache/
*/

#define _XOPEN_SOURCE       // strptime(3)


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>      // struct sockaddr_in
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>          // gethostbyname
#include <time.h>           // time(2) return the number of seconds since the Epoch
                            // localtime(3) broke down eoicg data in its components

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
    
    // HTTP DATE
    // Thu, 17 Oct 2019 07:18:26 GMT
    // %a, %d %b %Y %T=%H:%M:%S GMT=%Z 
    char* format = "%a, %d %b %Y %T %Z";     // man 3 strptime

    if ((cached = fopen(cacheName, "r")) != NULL) { 
        printf("\n----->Resource already downloaded, verify cache\n");
        
        // read first line of cached file, contains timestamp of download date
        // assume timestamp is a 10 character epoch/timestamp (valid until Saturday 20 November 2286)
        char cacheTime[11];
        fread(cacheTime, sizeof(char), 10, cached);
        cacheTime[10] = 0;
        time_t epochSaved = atoi(cacheTime);
        struct tm tmSaved = *gmtime(&epochSaved);
        char httpDateCache[30];
        strftime(httpDateCache, sizeof(httpDateCache), format, &tmSaved);
        printf("Last time of download is %s\n", httpDateCache);
        
        /// 1) request HEAD for the resource
        sprintf(request, "HEAD %s HTTP/1.1\r\nHost:%s\r\nIf-Modified-Since:%s\r\n\r\n", resourceName, hostname, httpDateCache);

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
                headers[headersCount].v = hResponse + i + 2;    // value of the header wo space
            }
        }

        // read the Last-Modified header
        int bodyLength = 0;
        char* lastModified = 0;
        int notModified = 0;
        if (!strcmp("304 Not Modified", headers[0].n+9)) {
            printf("Not modified, serve directly from cache\n");
            notModified = 1;
        } else {
           printf("Resource modified, need to be downloaded\n");
        }

        for (int i=1; i<headersCount; i++)
            if (!strcmp("Last-Modified", headers[i].n))
                lastModified = headers[i].v;

        printf("Last-Modified date %s\n", lastModified);
        // Last-Modified to epoch [ms]
        struct tm* httpTime = malloc(sizeof(struct tm));
        strptime(lastModified, format, httpTime); // string -> tm
        time_t epochRemote = mktime(httpTime); // tm -> epoch
        
        printf("epoch Remote %lu\n", (unsigned long)epochRemote);
        printf("epoch Cached %lu\n", (unsigned long)epochSaved);

        /// 2.a) if date in Last-Modified header is <= than cached file timestamp, the cache is valid
        if (notModified || (epochRemote <= epochSaved)) {
            printf("\n----->Serve from cache!!!");
            int c=0;
            char* fileBuff[1024*5];
            for (int l=0; (c = fread(fileBuff, sizeof(char), 1024*5, cached)) > 0; l += c)
               printf("%s\n", fileBuff);   // show to the user the cached file
    
            fclose(cached);
            return 0;
        }
        fclose(cached);
    } 

    /// 3) download the resource and save it in dir ./cache/    
    printf("\n----->Resource need to be downloaded\n");
    char* lastModified = 0;
    
    /// 3.a) download resource
    sprintf(request, "GET %s HTTP/1.1\r\nHost:%s\r\n\r\n", resourceName, hostname);
    write(s, request, strlen(request));
    
    int headersCount = 0;
    bzero(headers, 100 * sizeof(struct Header));
    bzero(hResponse, 1024*1024);

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
            headers[headersCount].v = hResponse + i + 2;    // value of the header
        }
    }
    // read the Content-Length, if available, else trasmission is chunked
    int bodyLength = 0;
    for (int i=1; i<headersCount; i++) {
        printf("%s: %s\n", headers[i].n, headers[i].v);
        if (!strcmp("Content-Length", headers[i].n))
            bodyLength = atoi(headers[i].v);
        
        if (!strcmp("Transfer-Encoding", headers[i].n) && !strcmp("chunked", headers[i].v))
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
    printf("\n----->Resource downloaded!!!\n%s\n", response);

    fclose(cached);
    return 0;
}
