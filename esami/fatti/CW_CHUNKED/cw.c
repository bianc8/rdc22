/*
Esercizio di Reti di Calcolatori

Si modifichi il programma che implementa il Web Client in modo che legga un entity body utilizzando il transfer coding chuncked, come descritto dalla grammatica riportata nella sezione 3.6.1 della RFC 2616, evitando tutte le parti opzionali.

Chunked-Body   = *chunk
                        last-chunk
                        trailer
                        CRLF

       chunk          = chunk-size [ chunk-extension ] CRLF
                        chunk-data CRLF
       chunk-size     = 1*HEX
       last-chunk     = 1*("0") [ chunk-extension ] CRLF

       chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
       chunk-ext-name = token
       chunk-ext-val  = token | quoted-string
       chunk-data     = chunk-size(OCTET)
       trailer        = *(entity-header CRLF)
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>  // atoi
#include <sys/socket.h> // socket, struct sockaddr
#include <sys/types.h>  // ""
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <string.h>  //strlen

struct sockaddr_in remote;
char response[10000001]; // messa in mem statica => init a 0, char terminatore di default

struct header {
   char * n;
   char * v;
} h[100];

int main() {
   int k,n;
   char * request = "GET / HTTP/1.1\r\nHost:www.google.it\r\n\r\n";
   char * statusline;
   char hbuffer[10000];
   unsigned char ipserver[4] = { 142, 250, 180, 3 };
   int s;

   if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0))) {
      printf("errno = %d\n", errno);
      perror("An error occured on creation of socket");
      return -1;
   }

   remote.sin_family = AF_INET;  // sa_family_t address family
   remote.sin_port = htons(80);  // in_port_t port in network byte order (16 bit)
   remote.sin_addr.s_addr = *((uint32_t *) ipserver); // struct_in_addr internet address { uint32_t s_addr // address in network byte order}

   if (-1 == connect(s, (struct sockaddr *)&remote, sizeof(struct sockaddr_in))) {
      perror("Connessione socket fallita");
      return -1;
   }
   int j = 0;  // scorre elementi in header
   statusline = h[0].n = hbuffer;
   int i = 0;  // scorre caratteri
   int chunked = 0;
   size_t len = 0;

   for(k=0; k < 1; k++){
      write(s, request, strlen(request));
      bzero(hbuffer,10000);   // azzera bytes
      statusline = h[0].n = hbuffer;
      int bodylen = 1000000;
      for (i=0, j=0; read(s, hbuffer + i, 1); i++) {
         if (hbuffer[i] == '\n' && hbuffer[i - 1] == '\r') {
            hbuffer[i-1] = 0;      // Termino il token attuale
            if (! h[j].n[0]) break;
            h[++j].n = hbuffer + i + 1;
         }
         if (hbuffer[i] == ':' && !h[j].v) {
            hbuffer[i] = 0;
            h[j].v = hbuffer + i + 1;
         }
      }
      
      for(i=1; i<j; i++){
         printf("name %s value %s\n", h[i].n, h[i].v);
         // if chunked transfer encoding
         if(!strcmp("Transfer-Encoding", h[i].n) && !strcmp(" chunked", h[i].v)) {
            chunked = 1; // chunked = true
         // if Content-Length
         } else if(!strcmp("Content-Length", h[i].n)) {
            bodylen = atoi(h[i].v);
            chunked = 0;
         }
      }
      
      char chunk_size_hex[100000];
      size_t chunk_size = 0;
      size_t cc = 0;
      // read (int fd, void* buf, size_t cont)
      if (chunked > 0) {
         // infinite loop to read chunks
         for (len = 0;;len += cc) {
            // read chunk_size until it meet CR LF
            for (i = 0; 0 < (n = read(s, chunk_size_hex + i, 1)); ++i) {
               if (chunk_size_hex[i] == '\n' && chunk_size_hex[i-1] == '\r') {
                  chunk_size_hex[i-1] = 0;
                  break;
               }
            }
            // convert hex string to size_t
            chunk_size = (size_t)strtol(chunk_size_hex, NULL, 16);
            printf("chunk_size %zu\n", chunk_size);
            // last chunk has chunk_size == 0
            if (chunk_size == 0)
               break;
            // read chunk data
            for (cc = 0; cc < chunk_size && 0 < (n = read(s, response + len + cc, chunk_size - cc)); cc += n);
            if (n < 0) {
               perror("Read fallita");
               return -1;
            }
            // read CR
            read(s, chunk_size_hex, 1);
            // read LF
            read(s, chunk_size_hex + 1, 1);
         }
      } else {
         for (len = 0; len<bodylen && (n = read(s, response + len, 10000000 - len)) > 0; len += n);
         if (n < 0) {
            perror("Read fallita");
            return -1;
         }
      }
      response[len] = 0;
      FILE *file = fopen("index.html", "w");
      int results = fputs(response, file);
      if (results == EOF)
         perror("Failed to write to index.html");
      fclose(file);
      printf("Response written in file index.html");
   }
   return 0;
}