/*
Esercizio di Reti di Calcolatori

Si modifichi il programma proxy in modo che per navigare su interent sia in HTTP che in HTTPS sia richiesta l’immissione di una login e una password tramite protocollo HTTP.

Si faccia riferimento alla RFC 1945 per il metodo di autenticazione basic.



s local
s2 (proxy server al client)
s3 (proxy client al server remote)


1) guardo se c'è header Authorization,
2) se c'è guardo se base64 dei miei utenti è uguale a questo header
3) se non c'è header o non c'è user:pwd, invio 401 Unauthorized
4) else invio request a s3, scrivo in s2

Per testare, apri da un altro terminale:
curl -v http://radioamatori.it/ -x 88.80.187.84:2088 -u "root:root"
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

char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char auth[] = "root:root";

/** f is an helper function for base64_encode
 *
 * @param unsigned char *a  pointer to input string to be converted
 * @param char *d           pointer to output string
 *
*/
void f(unsigned char *a, char *d){
	d[0] = base64[a[0] >> 2];
	d[1] = base64[((a[0] << 4) & 0x30) | (a[1] >> 4)];
	d[2] = base64[((a[1] << 2) & 0x3C) | (a[2] >> 6)];
	d[3] = base64[a[2] & 0x3F];
}

/** base64_encode encode a string to base64
 *
 * @param   unsigned char *in        pointer to input string to be converted
 * @param   int in_len               len of input string
 * @param   char *out                pointer to output string
 * 
*/
void base64_encode(unsigned char *in, int in_len, char *out){
	int out_offset = 0;
	int i;
	for(i=0; i<in_len-2; i+=3){
		f(in + i, out + out_offset);
		out_offset += 4;
	}
	if(in_len % 3 == 2){
		unsigned char x[3];
		x[0] = in[i];
		x[1] = in[i+1];
		x[2] = 0;
		f(x, out + out_offset);
		out[out_offset + 3] = '=';
	}
	else if(in_len % 3 == 1){
		unsigned char x[3];
		x[0] = in[i];
		x[1] = 0;
		x[2] = 0;
		f(x, out + out_offset);
		out[out_offset + 2] = out[out_offset + 3] = '='; 
	}
	out_offset += 4;
	out[out_offset] = 0;
}

int main()
{
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
local.sin_port = htons(2088);
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
printf("Remote address: %.8X\n",remote.sin_addr.s_addr);
if (fork()) continue;
if(s2 == -1){perror("Accept fallita"); exit(1);}
bzero(hbuffer,10000);
bzero(h,100*sizeof(struct header));
reqline = h[0].n = hbuffer;
for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
	printf("%c",hbuffer[i]);
  if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r'){
    hbuffer[i-1]=0; // Termino il token attuale
   if (!h[j].n[0]) break;
   h[++j].n=hbuffer+i+1;
  }
  if (hbuffer[i]==':' && !h[j].v && j>0){
    hbuffer[i]=0;
    h[j].v = hbuffer + i + 2;
  }
 }

char *authHeader = 0; 
for (i=1; i<j; i++) {
   printf("%s: %s\n", h[i].n, h[i].v);
   if (!strcmp(h[i].n, "Authorization")) {
      authHeader = h[i].v + strlen("Basic ");
      break;
   }
}

   if (!authHeader) {
      printf("Non c'è l'header auth\n");
      sprintf(response,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\nContent-Length:0\r\n\r\n");
    	write(s2, response, strlen(response));
      close(s2);
      continue;
   } else {
      printf("c'è l'header\n");
      char authENC[30];
      base64_encode((unsigned char*)auth, strlen(auth), authENC);
      
      if (!strcmp(authENC, authHeader)) {
            printf("AUTORIZZATO\n");
      } else {
         printf("NON AUTH\n");
         sprintf(response,"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\nContent-Length:0\r\n\r\n");
		 write(s2, response, strlen(response));
         close(s2);
         continue;
      }
   }


	method = reqline;
	for(i=0;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
	url=reqline+i;
	for(;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
	ver=reqline+i;
	for(;i<100 && reqline[i]!='\r';i++); reqline[i++]=0; 

   
   if ( !strcmp(method,"GET")){
		scheme=url;
			// GET http://www.aaa.com/file/file 
		for(i=0;url[i]!=':' && url[i] ;i++);
		if(url[i]==':') url[i++]=0;
		else {printf("Parse error, expected ':'"); exit(1);}
		if(url[i]!='/' || url[i+1] !='/') 
		{printf("Parse error, expected '//'"); exit(1);}
		i=i+2; hostname=url+i;
		for(;url[i]!='/'&& url[i];i++);	
		if(url[i]=='/') url[i++]=0;
		else {printf("Parse error, expected '/'"); exit(1);}
		filename = url+i;
		printf("Schema: %s, hostname: %s, filename: %s\n",scheme,hostname,filename); 

		he = gethostbyname(hostname);
		printf("%d.%d.%d.%d\n",(unsigned char) he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2],(unsigned char) he->h_addr[3]); 
		if (( s3 = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
			{ printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1); }

		server.sin_family = AF_INET;
		server.sin_port =htons(80);
		server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

		if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in)))
				{perror("Connect Fallita"); exit(1);}	
		sprintf(request,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",filename,hostname);
		printf("%s\n",request);
		write(s3,request,strlen(request));
		while (t = read(s3,buffer,2000))
			write(s2,buffer,t);
		close(s3);
		}
	else if(!strcmp("CONNECT",method)) { // it is a connect  host:port 
		hostname=url;
		for(i=0;url[i]!=':';i++); url[i]=0;
		port=url+i+1;
		printf("hostname:%s, port:%s\n",hostname,port);
		he = gethostbyname(hostname);
		if (he == NULL) { printf("Gethostbyname Fallita\n"); return 1;}
		printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]); 			
		s3=socket(AF_INET,SOCK_STREAM,0);

		if(s3==-1){perror("Socket to server fallita"); return 1;}
		server.sin_family=AF_INET;
		server.sin_port=htons((unsigned short)atoi(port));
	 	server.sin_addr.s_addr=*(unsigned int*) he->h_addr;			
		t=connect(s3,(struct sockaddr *)&server,sizeof(struct sockaddr_in));		
		if(t==-1){perror("Connect to server fallita"); exit(0);}
		sprintf(response,"HTTP/1.1 200 Established\r\n\r\n");
		write(s2,response,strlen(response));
			// <==============
		if(!(pid=fork())){ //Child
			while(t=read(s2,request2,2000)){	
				write(s3,request2,t);
			//printf("CL >>>(%d)%s \n",t,hostname); //SOLO PER CHECK
				}	
			exit(0);
			}
		else { //Parent	
			while(t=read(s3,response2,2000)){	
				write(s2,response2,t);
			//printf("CL <<<(%d)%s \n",t,hostname);
			}	
			kill(pid,SIGTERM);
			close(s3);
			}	
		}	
	else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
	}
	close(s2);
	exit(1);
}
close(s);
}
