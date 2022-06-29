/*
Esame di Reti di Calcolatori -  2 settembre 2016

Implementare un server HTTP che:

- reindirizza a una pagina predefinita se la risorsa di destinazione non è disponibile 
- Invii una risposta temporaneamente non disponibile se la risorsa è disponibile, e dopo una seconda richiesta dia l'output.

**Suggerimenti**:
L'header `Retry-After` è ignorata dalla maggior parte dei browser web, quindi il reindirizzamento non avverrà dopo 10 secondi, ma immediatamente.

Pseudo:
if (fin == NULL) --> 301 Moved Permanently Location: /404.html
else -->
   se è la prima volta che richiedo la risorsa, 307 Moved Temporarily
   else 200 OK
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
char * filename, fileCont[100];
FILE * fin;
int c;
int n;
int i,j,t, s,s2;
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
    h[j].v = hbuffer + i + 1;
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
         printf("----> File %s not found, Redirect to 404.html\n", filename);
         sprintf(response,"HTTP/1.1 301 Moved Permanently\r\nLocation: /404.html\r\n\r\n");
			write(s2,response,strlen(response));
			}
		else { 
         if (fileCont != NULL)
            printf("####### fileCont %s filename %s\n", fileCont, filename);
         // if file is 404.html or 307.html or fileCont is valid, return the requested file
         if (!strcmp(filename, "404.html") || !strcmp(filename, "307.html") || (fileCont != NULL && !strcmp(fileCont, filename))) {
			      printf("----> Return file: %s\n", filename);
               sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
               write(s2,response,strlen(response));
               while ( (c = fgetc(fin))!=EOF) write(s2,&c,1);
               fclose(fin);
         } 
         // else return risorsa non disponibile temporaneamente
         else {
               sprintf(fileCont, "%s", filename);
               printf("----> Set fileCont = %s and redirect to 307.html\n", fileCont);
               sprintf(response,"HTTP/1.1 307 Moved Temporarily\r\nLocation: /307.html\r\n\r\n");
               write(s2,response,strlen(response));
         }
	   } 
   }
	else {
		sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
	}
	close(s2);
}
close(s);
}
