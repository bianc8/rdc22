/* 
Esame 2015-07-03
Si modifichi il programma che implementa il web server in modo che questo, 
non appena riceve dal client una request per la risorsa corrispondente al path “/reflect”, 
anziché cercare un file da aprire ed inviare, invii al client una response nella quale l’entity body sia 

a. il testo esatto corrispondente all’intera request inviata dal client al server, comprensiva di tutti gli elementi che la compongono 
b. <CRLF>
c. L’indirizzo IP in notazione decimale puntata da cui il client ha inviato la propria request
d. <CRLF>
e.il port da cui il client ha effettuato la propria request
*/
/* 
- prima di aprire il file controllo se il client richiede una risorsa con url al path "/reflect". 
- Se il path corrisponde devo inviare l'entity body come specificato nel pdf.
- invio l'intera request mandata dal client verso il server e aggiungo un crlf finale.
- Invio l'indirizzo IP in notazione decimale puntata da cui il client ha inviato la request e aggiungo un CRLF finale.
- Invio il port con cui il client ha effettuato la request.
- Ho concluso l'invio dei dati.
- In caso la path non sia "/reflect" il server lavora in modo ordinario.
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
// array di char per salvare IP e port del client
char ipClient[30];

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
int i,j,t, s,s2;
int yes = 1;
int len;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
local.sin_family = AF_INET;
local.sin_port = htons(17999);
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
      // se path richiesto è "/reflect" allora rispondi con quanto descritto nel pdf
      if (!strncmp(url, "/reflect", strlen("/reflect"))) {
         // Salva in ipClient l'IP e il port del client
         sprintf(ipClient, "%d.%d.%d.%d\r\n%d", *((unsigned char *) &remote.sin_addr.s_addr), *((unsigned char *) &remote.sin_addr.s_addr+1), *((unsigned char *) &remote.sin_addr.s_addr+2), *((unsigned char *) &remote.sin_addr.s_addr+3), ntohs(remote.sin_port));
         // Salva in response la risposta del server.
         sprintf(response, "HTTP/1.1 200 OK\r\n\r\n%s %s %s\r\n%s", method, url, ver, ipClient);
         // scrivo nel file descriptor s2 (client) la risposta
         write(s2, response, strlen(response));
      }
      // casi di richiesta ordinari
      else { 
		filename = url+1;
		fin=fopen(filename,"rt");
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
			write(s2,response,strlen(response));
			}
		else{
			sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
			write(s2,response,strlen(response));
			while ( (c = fgetc(fin))!=EOF) write(s2,&c,1);
			fclose(fin);
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
