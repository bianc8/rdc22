/*
Implementare webserver, simile cgi.c, in modo compliant il cgi.
Avremo bisogno che il webserver:
    invii stdinput al programma, (dalla richiesta con metodo post ricevo entity body di un file)
    riceva variabili environment dal web server
    chiamato dal webserver
    stdout diventa html che il programma produce, viene inviato al client


Il client accede dal browser, c'è un form html:
    prende due stringhe come valori: stringa1 e stringa 2
    e bottone sfoglia che fa la submit di un file (richiesta post con il file nell'entity body)
    in output, sostituisce in input_file, ogni occorenza di stringa1 viene sostituita con stringa2

*/
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>



int pid;
struct sockaddr_in local, remote, server;
char request[10000];
char request2[10000];
char response[1000];
char response2[10000];

struct header {
	char * n;
	char * v;
} h[100]; 

struct hostent * he;

int main() {
	char hbuffer[10000];
	char buffer[2000];
	char * reqline;
	char * method, *url, *ver, *scheme, *hostname, *port;
	char * filename;
	FILE * fin;
	int c;
	int n;
	int i,j,t, s,s2,s3;
	int yes = 1;
	int len;
	if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
		{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
	local.sin_family = AF_INET;
	local.sin_port = htons(17999);
	local.sin_addr.s_addr = 0;

	t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	if (t == -1) {
		perror("setsockopt fallita"); return 1;
	}

	if (-1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in))) {
		perror("Bind Fallita"); return -1;
	}

	if (-1 == listen(s,10)) {
		perror("Listen Fallita"); return -1;
	}
	
	remote.sin_family = AF_INET;
	remote.sin_port = htons(0);
	remote.sin_addr.s_addr = 0;
	len = sizeof(struct sockaddr_in);
	while (1){
		s2=accept(s,(struct sockaddr *)&remote,&len);
		printf("Remote address: %.8X\n",remote.sin_addr.s_addr);
		if (fork())	// fork return non null in the father process
			continue;
		if(s2 == -1){
			perror("Accept fallita"); exit(1);
		}
		
		// Clean headers
		bzero(hbuffer,10000);
		bzero(h, 100*sizeof(struct header));
		
		reqline = h[0].n = hbuffer;
		for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
			printf("%c",hbuffer[i]);
			if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r') {
				hbuffer[i-1]=0; // Termino il token attuale
				if (!h[j].n[0])
					break;
				h[++j].n = hbuffer+i+1;
			}
			if (hbuffer[i]==':' && !h[j].v && j>0) {
				hbuffer[i]=0;
				h[j].v = hbuffer + i + 1;
			}
		}

		printf("Request line: %s\n",reqline);
		method = reqline;
		for(i=0;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
		url=reqline+i;
		for(;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
		ver=reqline+i;
		for(;i<100 && reqline[i]!='\r';i++); reqline[i++]=0; 
		
		// method GET
		if (!strcmp(method,"GET")){
			scheme=url;
			printf("url=%s\n",url);
			
			for(i=0;url[i]!=':' && url[i] ;i++);
			// GET schema://hostname/file

			// consumo :
			if(url[i]==':')
				url[i++]=0;
			else {
				printf("Parse error, expected ':'"); exit(1);
			}
			// consumo //
			if(url[i]!='/' || url[i+1] !='/') {
				printf("Parse error, expected '//'"); exit(1);
			}
			
			// dopo uno slash c'è l'hostname 
			i=i+2;
			hostname=url+i;
			for(;url[i]!='/'&& url[i];i++);	
			if(url[i]=='/')
				url[i++]=0;
			else {
				printf("Parse error, expected '/'"); exit(1);
			}
			
			// dopo l'ultimo slash c'è il filename
			filename = url+i;
			printf("Schema: %s, hostname: %s, filename: %s\n",scheme,hostname,filename); 

			he = gethostbyname(hostname);
			printf("%d.%d.%d.%d\n",(unsigned char) he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2],(unsigned char) he->h_addr[3]); 
			
			if (-1 == ( s3 = socket(AF_INET, SOCK_STREAM, 0 ))) {
				printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1);
			}

			server.sin_family = AF_INET;
			server.sin_port =htons(80);
			server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

			if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in))) {
				perror("Connect Fallita"); exit(1);
			}

			sprintf(request,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",filename,hostname);
			printf("%s\n",request);
			
			write(s3,request,strlen(request));
			while ( t=read(s3,buffer,2000))
				write(s2,buffer,t);
			close(s3);
		}
		// For other method than GET/CONNECT
		else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
			write(s2,response,strlen(response));
		}
		close(s2);
		exit(1);
	}
	close(s);
}
