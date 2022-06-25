/*
# Esame di Reti di Calcolatori - 3 Settembre 2020

Si modifichi il programma Web Server sviluppato durante il corso in modo tale che vieti, tramite opportuno codice di errore (cfr. RFC2616, cap. 10.4 ), ad un user agent (browser) convenzionale di accedere alla risorsa /file2.html se prima il medesimo user agent non abbia precedentemente avuto accesso alla risorsa /file1.html.

Per implementare questa funzione si utilizzino i meccanismi di gestione dello stato HTTP, detti Cookies, facendo riferimento agli esempi della sezione 3.1 della RFC6265 e alle grammatiche nella Sezione 4 del medesimo documento.

Il meccanismo dei Cookie può essere visto come una variante del meccanismo di autenticazione visto a lezione, in quanto, al posto di richiedere al client di includere in ogni request l’header WWW-authenticate riportante credenziali (username e password) precedentemente registrate sul server, similmente richiede al client di includere, in ogni request, l’header Cookie riportante una il nome e il valore di una variabile di stato comunicata dal server al client al primo accesso. Questo permette al server di capire che tante richieste provengono da uno stesso user agent e condividono uno stesso stato, senza necessità di gestire la registrazione di utenti.
N.B. lo user agent non sarà obbligato ad accedere al /file1.html subito prima di accedere al /file2.html. Al contrario il server deve poter permettere anche la sequenza di accesso
/file1.html, ...<altre risorse>..., /file2.html.
  
## Agli studenti più preparati si richiede un requisito ulteriore:
  
Non sarà sufficiente allo user agent accedere una volta per tutte a /file1.html per poi aver il permesso di accedere tante volte al /file2.html. Al contrario, dopo aver dato il permesso di accedere al /file2.html, il server vieterà un secondo accesso se prima il medesimo user agent non avrà avuto accesso di nuovo al /file1.html.


403 Forbidden\r\nLocation:/file1.html\r\n


Ad ogni richiesta a file1.html viene aggiunto un nuovo cookie daDoveProviene=provieneDa1.
		
Ad ogni richiesta a file2.html viene controllato se esiste il cookie daDoveProviene e se ha valore "provieneDa1"

Quando il server riceve una richiesta;
1) se tra gli header della richiesta c'è l'header Cookie: daDoveProviene=xyz;
    1.a) e se la risorsa richiesta è file2.html
		verifica che il cookie sia daDoveProviene=provieneDa1
        1.a.1) se l'user agent è bloccato --> invia 403 Forbidden Location: /file1.html
        1.a.2) se non è bloccato --> invia la risorsa richiesta e nelle successive richieste a file2.html, non terrà conto del cookie

2) se non è presente l'header Cookie nella richiesta
    2.a) Aggiungi il cookie daDoveProviene=provieneDa1
    2.b) Se la risorsa richiesta è file1.html  --> Invia la risorsa richiesta
    2.c) Se la risorsa richiesta è file2.html --> Invia 403 Forbidden Location: /file1.html
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
	int blocked = 0;

char hbuffer[10000];
char * reqline;
char * method, *url, *ver;
char * filename;
FILE * fin;
int c;
int n;
int i,j,t,k, s,s2;
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
    h[j].v = hbuffer + i + 2;
  }
 }

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
		bzero(response, 1000);
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
			write(s2,response,strlen(response));
			}
		else{
			if (!strncmp(filename, "file2.html", strlen("file2.html"))) {
				// parse Cookie header to extract cookieKey and CookieValue
				char *cookieKey, *cookieValue;
				for (i=0; i<j; i++) {
					if (!strcmp(h[i].n, "Cookie")) {
						cookieKey = h[i].v;
						for (k=0; cookieKey[k] != '='; k++);
						cookieKey[k++]=0;
						cookieValue = h[i].v + k;
						break;
					}
				}
				// request blocked to file2.html
				if (blocked || strcmp(cookieKey, "daDoveProviene") || strcmp(cookieValue, "provieneDa1")) {
					printf("Blocked request\n");
					sprintf(response,"HTTP/1.1 403 Forbidden\r\n\r\n<!DOCTYPE html><html><body><p>User agent not allowed</p><br/><a href=\"./file1.html\">Please visit page ./file1 to gain access</a></body></html>");	
					write(s2,response,strlen(response));
					continue;
				}
				// request not blocked to file2.html
				else {
					printf("Serve request because cookie %s=%s and request not blocked\n", cookieKey, cookieValue);
					sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
					blocked = 1;
				}	
			} 
			// request to file1 --> Set-Cookie
			else if (!strncmp(filename, "file1.html", strlen("file1.html"))) {
				printf("Requested file1.html, cookie setted\n");
				sprintf(response, "HTTP/1.1 200 OK\r\nSet-Cookie: daDoveProviene=%s\r\n\r\n", "provieneDa1");
				blocked = 0;
			} else {
				sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
			}
			write(s2,response,strlen(response));
			while ( (c = fgetc(fin))!=EOF) write(s2,&c,1);	
		
			fclose(fin);
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
