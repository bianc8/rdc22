/*
Esercizio di Reti di calcolatori

Si modifichi il programma proxy in modo tale che il proxy consente
 solo il passaggio verso il server 88.80.187.84, 1.1.1.1 e
  i server di Google e Unipd (un unico indirizzo a testa)


Pseudocodice
  in s3 controllo se l'ip del server remoto è consentito
    se sì invio la richiesta al server remoto e scrivo la risposta sul client
    else invio al client 502 Bad Gateway
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


char *whitelist[] = {"88.80.187.84", "1.1.1.1"};

int pid;
struct sockaddr_in local, remote, server;
char request[10000];
char request2[10000];
char response[1000];
char response2[10000];

struct header {
  char * n;
  char * v;
} h[100], hs[100];

struct hostent *he;


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
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1) { printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
local.sin_family = AF_INET;
local.sin_port = htons(17999);
local.sin_addr.s_addr = 0;

t= setsockopt(s, SOL_SOCKET,SO_REUSEADDR,&yes, sizeof(int));
if (t==-1){ perror("setsockopt fallita"); return 1;}

if (-1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in))) { perror("Bind Fallita"); return -1;}

if (-1 == listen(s,10)) { perror("Listen Fallita"); return -1;}

remote.sin_family = AF_INET;
remote.sin_port = htons(0);
remote.sin_addr.s_addr = 0;
len = sizeof(struct sockaddr_in);
while (1){
	s2=accept(s,(struct sockaddr *)&remote,&len);
	if (fork()) continue;
	if(s2 == -1){perror("Accept fallita"); exit(1);}
	
	bzero(hbuffer,10000);
	bzero(h, 100*sizeof(struct header));
	reqline = h[0].n = hbuffer;
	
	for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
		printf("%c",hbuffer[i]);
		if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r') {
            hbuffer[i-1]=0; // Termino il token attuale
			if (!h[j].n[0])
                break;
            h[++j].n=hbuffer+i+1;
        }
		if (hbuffer[i]==':' && !h[j].v && j>0){
			hbuffer[i]=0;
			h[j].v = hbuffer + i + 2;
		}
	}

	printf("Request line: %s\n",reqline);
	method = reqline;
	for(i=0;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
	url=reqline+i;
	for(;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
	ver=reqline+i;
	for(;i<100 && reqline[i]!='\r';i++); reqline[i++]=0; 
	
    if (!strcmp(method,"GET")){
        scheme=url;
        // GET http://www.aaa.com/file/file 
        for(i=0;url[i]!=':' && url[i] ;i++);
        if(url[i]==':')
            url[i++]=0;
        else {
            printf("Parse error, expected ':'");
            exit(1);
        }
        if(url[i]!='/' || url[i+1] !='/') {
            printf("Parse error, expected '//'");
            exit(1);
        }
        i=i+2;
        hostname=url+i;
        for(;url[i]!='/'&& url[i];i++);	
        if(url[i]=='/')
            url[i++]=0;
        else {
            printf("Parse error, expected '/'");
            exit(1);
        }
        
        filename = url+i;
        //printf("Schema: %s, hostname: %s, filename: %s\n",scheme, hostname, filename); 

        he = gethostbyname(hostname);
        
        if (-1 == ( s3 = socket(AF_INET, SOCK_STREAM, 0 ))) {
            printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1);
        }

        server.sin_family = AF_INET;
        server.sin_port =htons(80);
        server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

        

        if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in))) {
            perror("Connect Fallita"); exit(1);
        }
        
        // controllo se l'ip del server remoto è in whitelist
        char remoteIp[20];
        int blocked = 1;
        int filterData = 0;
        sprintf(remoteIp, "%d.%d.%d.%d", (unsigned char) he->h_addr[0], (unsigned char) he->h_addr[1], (unsigned char) he->h_addr[2], (unsigned char) he->h_addr[3]);

        // check if client ip is allowed to transmit
        for (int l=0; whitelist[l]; l++) {
            if (!strncmp(remoteIp, whitelist[l], strlen(whitelist[l]))) {
                printf("Client is allowed to transmit data to %s aKa %s\n", hostname, remoteIp);
                blocked = 0;
                break;
            }
        }

        if (!blocked) {
            sprintf(request, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n", filename, hostname);
            write(s3,request,strlen(request));
            
            bzero(buffer, 2000);
            // finish reading entity-body from remote server and write to client
            while (t = read(s3, buffer, 2000))
                write(s2, buffer, t);
        } else {
            printf("Blocked remote website: %s aKa %s\n", hostname, remoteIp);
            sprintf(response,"HTTP/1.1 502 Bad Gateway\r\nContent-Length:0\r\nConnection:close\r\n\r\n");
            write(s2, response, strlen(response));
            break;
        }
        
        close(s3);
    }
    // it is a connect  host:port 
    else if(!strcmp("CONNECT",method)) {
        hostname=url;
        for(i=0; url[i] != ':'; i++);
        url[i]=0;
        port=url+i+1;

        printf("hostname:%s, port:%s\n",hostname,port);
        he = gethostbyname(hostname);
        if (he == NULL) {
            printf("Gethostbyname Fallita\n"); return 1;
        }
        
        printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]); 			
        s3=socket(AF_INET,SOCK_STREAM,0);

        if(s3==-1){
            perror("Socket to server fallita"); return 1;
        }
        
        server.sin_family=AF_INET;
        server.sin_port=htons((unsigned short)atoi(port));
        server.sin_addr.s_addr=*(unsigned int*) he->h_addr;			
        t=connect(s3,(struct sockaddr *)&server,sizeof(struct sockaddr_in));		
        if (t == -1){
            perror("Connect to server fallita"); exit(0);
        }
        sprintf(response,"HTTP/1.1 200 Established\r\n\r\n");
        write(s2,response,strlen(response));
            // <==============
        if(!(pid=fork())){ //Child
            while(t=read(s2,request2,2000)){	
                write(s3,request2,t);
                //printf("CL >>>(%d)%s \n",t,hostname); //SOLO PER CHECK
            }	
            exit(0);
        } else { //Parent	
            while(t=read(s3,response2,2000)){	
                write(s2,response2,t);
                //printf("CL <<<(%d)%s \n",t,hostname);
            }	
            kill(pid,SIGTERM);
            close(s3);
        }	
    } else {
        sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
        write(s2,response,strlen(response));
    }

	close(s2);
	exit(1);
}
close(s);
}
