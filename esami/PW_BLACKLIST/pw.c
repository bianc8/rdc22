/*
Si modifichi il programma wp16.c in modo tale che:
se l'indirizzo IP del client che vi si collega e' presente in una lista di indirizzi IP memorizzata nel programma wp16.c (massimo 4 indirizzi)
allora il proxy consente solo il passaggio di file con contenuto testo o html.

s
s2 client -> proxy
s3 proxy -> server remoto

 da s2 controllo l'ip se è in blacklist
    se è in blacklist:
        da s3 controllo header Content-Type: text/plain o text/html
            se è di quel tipo --> invia la response al client
            se non è di quel tipo --> non invia la reponse al client (401 Unauthorized)
    se non è in blacklist:
        invia la response al client in ogni caso
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



char *blacklist[] = {"192.168.1.1", "192.168.1.2", "192.168.1.3", "127.0.0.1"};

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

struct hostent * he;


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
        printf("url=%s\n",url);
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
        printf("Schema: %s, hostname: %s, filename: %s\n",scheme, hostname, filename); 

        he = gethostbyname(hostname);
        printf("%d.%d.%d.%d\n", (unsigned char) he->h_addr[0], (unsigned char) he->h_addr[1], (unsigned char) he->h_addr[2], (unsigned char) he->h_addr[3]); 
        if (-1 == ( s3 = socket(AF_INET, SOCK_STREAM, 0 ))) {
            printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1);
        }

        server.sin_family = AF_INET;
        server.sin_port =htons(80);
        server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

        if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in))) {
            perror("Connect Fallita"); exit(1);
        }
        
        sprintf(request, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n", filename, hostname);
        printf("%s\n",request);
        write(s3,request,strlen(request));
        
        // controllo se l'ip di s2 è in blacklist
        char ip_client[30];
        int blocked = 1;
        int filterData = 0;
        sprintf(ip_client, "%d.%d.%d.%d",
            *((unsigned char*) &remote.sin_addr.s_addr),
            *((unsigned char*) &remote.sin_addr.s_addr+1),
            *((unsigned char*) &remote.sin_addr.s_addr+2),
            *((unsigned char*) &remote.sin_addr.s_addr+3)
        );

        // check if client ip is allowed to transmit
        for (int l=0; blacklist[l]; l++) {
            if (!strncmp(ip_client, blacklist[l], strlen(blacklist[l]))) {
                printf("Client ip %s is allowed to send only text or html\n", ip_client);
                filterData = 1;
                break;
            }
        }

        char hserver[10000];
        char buffer_cp[1024*1024];
        int length=0;
        
        bzero(buffer_cp, 1024*1024);
        bzero(hserver,10000);
        bzero(hs, 100 * sizeof(struct header));

        reqline = hs[0].n = hserver;    // IMPORTANT

        // read headers from remote server
        for (i=0,j=0; t = read(s3, buffer_cp+i, 1); i++) {
            length += t;
            strncpy(hserver+i, buffer_cp+i, t);

            if(hserver[i]=='\n' && hserver[i-1]=='\r'){
                hserver[i-1]=0;
                if (!hs[j].n[0])
                    break;                    
                hs[++j].n = hserver+i+1;
            }
            if (hserver[i]==':' && !hs[j].v && j>0){
                hserver[i]=0;
                hs[j].v = hserver + i + 2;
                printf("%s : %s", hs[j].n, hs[j].v);
            }
        }
        
        // parsing headers from remote server
        for (int l=1; l<j; l++) {
            printf("------s3 HEADERS: %s : %s\n", hs[l].n, hs[l].v);
            if (!strcmp("Content-Type", hs[l].n)) {
                if (filterData) {
                    if (!strncmp("text/plain", hs[l].v, strlen("text/plain")) || !strncmp("text/html", hs[l].v, strlen("text/html")))
                        blocked = 0;
                } else {
                    blocked = 0;
                }
                
            }
        }

        if (!blocked) {
                // write headers previously read
                write(s2, buffer_cp, length);
            
                bzero(buffer, 2000);
                // finish reading entity-body from remote server and write to client
                while (t = read(s3, buffer, 2000))
                    write(s2, buffer, t);
        } else {
            printf("Blocked Content type\n");
            sprintf(response,"HTTP/1.1 401 Unauthorized\r\nContent-Length:0\r\nConnection:close\r\n\r\n");
            write(s2,response,strlen(response));
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
