/*
Esame di Reti di Calcolatori - 12 luglio 2018

Si modifichi il programma ws.c in modo tale che sia in grado di gestire più richieste HTTP/1.1 
all'interno della medesima connessione TCP senza l'uso di chunk.
Si gaccia riferimento alla RFC2616.
Si provi con un browser l'accesso a due pagine tra loro linkate verificando che il server risponda 
a numerose richieste senza interrompere la connessione.

Pseudocodice:
apro un thread una volta che ho fatto l'accept per ogni nuova connessione

ogni processo figlio itera le varie read finche non riceve in s2 MSG_PEEK o MSG_DONTWAIT

 */

#include<stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>

struct sockaddr_in local, remote;
char request[1000001];
char response[1000];

struct header {
  char * n;
  char * v;
} h[100];


int main()
{
char hbuffer[10000];
char * reqline;
char * method, *url, *ver;
char * filename;
FILE * fin;
int c;
int n;
int i,j,t, s,s2, ka;
int yes = 1;
int len;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
local.sin_family = AF_INET;
local.sin_port = htons(9088);
local.sin_addr.s_addr = 0;

t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
if (t==-1){perror("setsockopt fallita"); return 1;}

if ( -1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in)))
{ perror("Bind Fallita"); return -1;}

if ( -1 == listen(s,10)) { perror("Listen Fallita"); return -1;}
remote.sin_family = AF_INET;
remote.sin_port = htons(0);
remote.sin_addr.s_addr = 0;
len = sizeof(struct sockaddr_in);
while ( 1 ){
	s2=accept(s,(struct sockaddr *)&remote,&len);
   printf("######## Accept nuova\n");
   if (fork())
      continue;
do {
   printf("## Open socket: %d\n", s2);
bzero(hbuffer,10000);
bzero(h,sizeof(struct header)*100);
reqline = h[0].n = hbuffer;
for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
  if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r'){
    hbuffer[i-1]=0; // Termino il token attuale
   if (!h[j].n[0]) break;
   h[++j].n=hbuffer+i+1;
  }
  if (hbuffer[i]==':' && !h[j].v){
    hbuffer[i]=0;
    h[j].v = hbuffer + i + 2;
  }
 }

   ka=0;
   for (i=0; i<j; i++) {
      //printf("# %s : %s\n", h[i].n, h[i].v);
      if (!strcmp(h[i].n, "Connection")) {
         ka = !strcmp(h[i].v, "keep-alive");
      }
   }

	printf("%s\n",reqline);
	if(len == -1) { perror("Read Fallita"); return -1;}
	method = reqline;
	for(i=0;reqline[i]!=' ';i++); reqline[i++]=0; 
	url=reqline+i;
	for(; reqline[i]!=' ';i++); reqline[i++]=0; 
	ver=reqline+i;
	for(; reqline[i]!=0;i++); reqline[i++]=0; 
	if ( !strcmp(method,"GET")){
		filename = url+1;
		fin=fopen(filename,"rt");
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
			write(s2,response,strlen(response));
			}
		else{ 
         int size = 0;
         while((c = fgetc(fin)) != EOF) size++;
			sprintf(response,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", size);
			write(s2,response,strlen(response));
		        rewind(fin);
			while ( (c = fgetc(fin))!=EOF) write(s2,&c,1);
			fclose(fin);
			}
	}
	else {
	   sprintf(response,"HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
    	write(s2,response,strlen(response));
	}
} while (ka && recv(s2, NULL, 1, MSG_PEEK | MSG_DONTWAIT));
//ssize_t  recv(int sockfd, void *buf, size_t len, int flags);
   printf("## Close socket %d\n", s2);
	close(s2);
}
close(s);
}
