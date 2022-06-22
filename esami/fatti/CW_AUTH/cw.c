/*
Esercizio di Reti di Calcolatori

Si modifichi il programma Web Client in modo tale che possa accedere ai contenuti privati di un web server attraverso l’immissione di una login e una password tramite protocollo HTTP.

Si faccia riferimento alla RFC 1945 per il metodo di autenticazione basic.

1) basta aggiungere l'header Authorization con l'encoding base64 dell'username e della password nel formato user:pwd
Es
admin:admin => Base64 => YWRtaW46YWRtaW4=
GET /secure/index.html HTTP/1.1 Host:88.80.187.84 Authorization: Basic YWRtaW46YWRtaW4=
*/


#include <stdlib.h>  // atoi
#include <stdio.h>
#include <string.h>  //strlen
#include <errno.h>
#include <sys/types.h>  // ""
#include <sys/socket.h> // socket, struct sockaddr
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <netdb.h>          // gethostbyname

struct sockaddr_in remote;
char request[1024];
char response[10000001]; // messa in mem statica => init a 0, char terminatore di default

struct header {
   char * n;
   char * v;
} h[100];


char hbuffer[1024*1024];

char hostname[1000];

char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void f(unsigned char *a, char *d) {
    d[0] = base64[a[0] >> 2];
    d[1] = base64[(a[0] << 4) & 0x30 | (a[1] >> 4)];
    d[2] = base64[((a[1] << 2) & 0x3C) | (a[2] >> 6)];
    d[3] = base64[a[2] & 0x3F];
}
void base64_encode(unsigned char *in, int in_len, char *out) {
    int i, out_offset = 0;
    for (i=0; i<in_len-2; i+=3) {
        f(in+i, out+out_offset);
        out_offset += 4;
    }
    if (in_len % 3 == 2) {
        unsigned char x[3];
        x[0] = in[i];
        x[1] = in[i+1];
        x[2] = 0;
        f(x, out+out_offset);
        out[out_offset+3] = '=';
    } else if (in_len % 3 == 1) {
        unsigned char x[3];
        x[0] = in[i];
        x[1] = 0;
        x[2] = 0;
        f(x, out+out_offset);
        out[out_offset+2] = out[out_offset+3] = '=';
    }
    out_offset+=4;
    out[out_offset] = 0;
}


int main() {
    int s;
    if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0 ))) {
        printf("errno = %d\n",errno); perror("Socket Fallita"); return -1;
    }

    // Specify the hostname that the client needs to connect to
    sprintf(hostname, "88.80.187.84");

    // Specify the resource that need to be downloaded
    char resourceName[] = "/secure/index.html";

    // Resolving the hostname to an IP address
    struct hostent* remoteIP;
    printf("Resolving IP for %s...\n\n", hostname);
    remoteIP = gethostbyname(hostname);
    
    
    char *line = "admin:admin",
        encodedLine[20];
    base64_encode((unsigned char*) line, strlen(line), encodedLine);
    
    /// 1) request GET for the resource
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nAuthorization: Basic %s\r\n\r\n", resourceName, hostname, encodedLine);

    // Assign the required information to the remote address object (?)
    remote.sin_family = AF_INET;
    remote.sin_port = htons(2088);
    remote.sin_addr.s_addr = *(unsigned int*)(remoteIP->h_addr_list[0]);

    // Open a connection on the socket created previously
    if (-1 == connect(s, (struct sockaddr *) &remote, sizeof(struct sockaddr_in))) {
        perror("Connect Fallita"); return -1;
    }

    int j = 0;  // scorre elementi in header
    int i = 0;  // scorre caratteri
    int n, k;
    size_t len = 0;

    for(k=0; k < 1; k++){
        write(s, request, strlen(request));
        bzero(hbuffer,10000);   // azzera bytes
        h[0].n = hbuffer;
        int bodylen = 1000000;
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
        
        printf("%s\n", h[0].n);
        for(i=1; i<j; i++){
            printf("%s : %s\n", h[i].n, h[i].v);
            if(!strcmp("Content-Length", h[i].n))
                bodylen = atoi(h[i].v);
        }
        
        for (len = 0; len<bodylen && (n = read(s, response + len, 10000000 - len)) > 0; len += n);
        if (n < 0) { perror("Read fallita"); return -1; }
        
        response[len] = 0;
        FILE *file = fopen("index.html", "w");
        int results = fputs(response, file);
        if (results == EOF)
            perror("Failed to write to index.html");
        fclose(file);
        printf("\nResponse written in file index.html\n%s\n", response);
    }
    return 0;
}