#include <stdio.h>
#include <errno.h>
#include <sys/socket.h> // socket, struct sockaddr
#include <sys/types.h>  // ""
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <string.h>  //strlen

struct sockaddr_in remote;
char response[1000001]; // messa in mem statica => init a 0, char terminatore di default

struct header {
   char * n;
   char * v;
} h[100];

int main() {
   size_t len = 0;
   int n;
   char * request = "GET / HTTP/1.0\r\n\r\n";
   char * statusline;
   char hbuffer[10000]
   unsigned char ipserver[4] = { 142, 250, 180, 3 };
   int s;

   if ((s = socket(AF_INET, SOCK_STREAM, 0) == -1)) {
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
  
   write(s, request, strlen(request));   
   statusline = h[0].n = response;

   int j = 0;  // scorre elementi in header
   int i = 0;  // scorre caratteri

   for (i=0,j=0; read(s, hbuffer + i, 1); i++) {
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
   for(i=1; i<j; i++) printf("%s ---> %s\n",h[i].n,h[i].v);

   //  read (int fd, void* buf,      size_t cont)
   for (size_t len = 0; (n = read(s,      response + len, 1000000 - len)) > 0; len += n);
   if (n == -1) {
      perror("Read fallita");
      return -1;
   }
   response[len] = 0;
   printf("Response = %s \n", response);
   // unix command to see active connections
   // netstat -tn 
   // stato della connessione: established / time_wait / close_wait

   // gateway scrive sul lv 4 il port univoco per rendere univoci i vari computer in locale
   return 0;
}