#include <stdio.h>
#include <sys/socket.h> // socket, struct sockaddr
#include <sys/types.h>  // ""
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <errno.h>
#include <stdint.h>
#include <unistd.h>     // write
#include <string.h>     // strlen

struct sockaddr_in local, remote;
char request[1000001];  // messa in mem statica => init a 0, char terminatore di default
char response[1000];

int main()
{
   char * method, *url, *ver;
   char * filename;
   FILE * fin;
   int c;
   int n;
   int i,t, s,s2;
   int yes = 1;
   int len;
   if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1) {
      printf("errno = %d\n",errno); perror("Socket Fallita"); return -1;
   }
   local.sin_family = AF_INET;   // sa_family_t address family
   local.sin_port = htons(7999); // in_port_t port in network byte order (16 bit)
                                 // can't use port 80 on web server because already in use
   local.sin_addr.s_addr = 0;    // struct_in_addr internet address { uint32_t s_addr }
                                 // address in network byte order

   t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
   if (t==-1){
      perror("setsockopt fallita"); return 1;
   }

   if ( -1 == bind(s, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) {
      perror("Bind Fallita"); return -1;
   }
   // int listen(int sockfd, int backlog)
   // backlog = max queue length (nMaxExpected connessioni / poisson)
   if ( -1 == listen(s, 10)) {
      perror("Listen Fallita"); return -1;
   }
   
   remote.sin_family = AF_INET;
   remote.sin_port = htons(0);
   remote.sin_addr.s_addr = 0;
   
   len = sizeof(struct sockaddr_in);
   // int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen, int flags)
   s2=accept(s,(struct sockaddr *)&remote,&len);
   len = read(s2,request,1000);

   if(len == -1) {
      perror("Read Fallita"); return -1;
   }
   
   // consumo e estraggo i token senza copiare byte
   method = request;
   
   for (i=0; i<len && request[i]!=' '; i++); request[i++]=0; 
   
   url=request+i;
   
   for(;i<len && request[i]!=' ';i++); request[i++]=0; 
   
   ver=request+i;
   
   for(;i<len && request[i]!='\r';i++); request[i++]=0; 
   
   if (!strcmp(method,"GET")){
      filename = url+1; // senza prendere il primo slash /index.html
      fin=fopen(filename,"rt");
      if (fin == NULL){ // scrive su stringa
         sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");// scrivo risposta file not found
         write(s2,response,strlen(response)); // scrivo stringa su socket s2
         }
      else{ 
         sprintf(response,"HTTP/1.1 200 OK\r\n\r\n"); // scrive su stringa
         write(s2,response,strlen(response));   // scrivo string su socket s2
         while ( (c = fgetc(fin))!=EOF) write(s2,&c,1); // copio file in response char by char
         fclose(fin);
      }
   }
   else {
      sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");// scrive su stringa Not Implemented
      write(s2,response,strlen(response)); // scrivo string su socket s2
   }
   close(s2);
   close(s);
}

// client si connette a indiirizzo ip:porta 7999
