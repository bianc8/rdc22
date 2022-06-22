/*
Esame di Reti di Calcolatori - 13 Luglio 2021

Si modifichi il programma del web proxy riportato nel file esame.c presente nella vostra cartella personale in modo tale che si comporti come segue 

1)	il web proxy, non appena riceverà dal web client una request di una risorsa, effettuerà a sua volta molte request al web server, ciascuna delle quali scaricherà un segmento dell’entity body della risorsa richiesto di lunghezza pari a 1000 bytes fino a che l’intero l’entity body risulterà scaricato (l’ultimo segmento avrà ovviamente una lunghezza ≤ 1000 bytes).

2)	Il web proxy invierà l’intero entity body della risorsa al web client tramite un’unica response, riportando così in un unico stream tutti i segmenti scaricati dal server nell’ordine corretto, sì che per il web client lo scaricamento a segmenti risulterà completamente trasparente. 

Al fine di implementare la funzione, si faccia riferimento all’header Range dell’HTTP/1.1 definito nella RFC 2616: sezioni 14.35, 3.12, 14.16 .


Per la sperimentazione collegarsi con il web client (configurato per utilizzare il proxy modificato) all’URL  http://88.80.187.84/image.jpg 

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
} h[100], hs[100];

struct hostent * he;

char buffer[1024*1024*1024];

int main() {
	char hbuffer[10000];
    char hbuffer_cp[10000];
	char * reqline;
	char * method, *url, *ver, *scheme, *hostname, *port;
	char * filename;
	FILE * fin;
	int c;
	int n;
	int i,j,t, l,k, s,s2,s3, bodylen;
	int yes = 1;
	int len;

    // RANGE
    int range = 0;
    int rangeSize = 50000;
    // Content-Range: tmp1-tmp2/size
    int tmp1,   // range
        tmp2;   // range + rangeSize - 1
    long size = 50000;   // total resource size

	if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1) {
		printf("errno = %d\n",errno); perror("Socket Fallita"); return -1;
	}
	
	local.sin_family = AF_INET;
	local.sin_port = htons(17999);
	local.sin_addr.s_addr = 0;

	t = setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
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
	
	while (1) {
		s2=accept(s,(struct sockaddr *)&remote,&len);
		printf("Remote address: %.8X\n",remote.sin_addr.s_addr);
		if (fork())
			continue;
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
		if ( !strcmp(method,"GET")){
			scheme=url;
            // GET http://www.aaa.com/file/file
			printf("url=%s\n",url);
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

            FILE * fout = fopen(filename, "wb");

			he = gethostbyname(hostname);
			printf("%d.%d.%d.%d\n\n",(unsigned char) he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2],(unsigned char) he->h_addr[3]); 

            // ## RANGE
            for (range=0; range<size; range+=rangeSize) {
			    if (( s3 = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
				    { printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1); }

			    server.sin_family = AF_INET;
			    server.sin_port =htons(80);
			    server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

			    if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in)))
					{perror("Connect Fallita"); exit(1);}	

                bzero(request, 10000);
			    sprintf(request,"GET /%s HTTP/1.1\r\nHost:%s\r\nRange:bytes=%d-%d\r\n\r\n",filename,hostname, range, range+rangeSize-1);

			    if (-1 == write(s3, request, strlen(request))) {
                    perror("write fallita\n"); return -1;
                }

                // parsing header
                bzero(hbuffer_cp,10000);   // azzera bytes
                bzero(hs, 100*sizeof(struct header));

                hs[0].n = hbuffer_cp;
                for (l=0, k=0; read(s3, hbuffer_cp + l, 1); l++) {
                    if (hbuffer_cp[l] == '\n' && hbuffer_cp[l - 1] == '\r') {
                        hbuffer_cp[l-1] = 0;      // Termino il token attuale
                        if (! hs[k].n[0]) break;
                        hs[++k].n = hbuffer_cp + l + 1;
                    }
                    if (hbuffer_cp[l] == ':' && !hs[k].v) {
                        hbuffer_cp[l] = 0;
                        hs[k].v = hbuffer_cp + l + 2;
                    }
                }

                bodylen = 1000000;
                for(l=1; l<k; l++){
                    if (!strcmp("Content-Length", hs[l].n)) {
                        bodylen = atoi(hs[l].v);
                    }
                    else if (!strcmp("Content-Range", hs[l].n)) {
                        sscanf(hs[l].v+6, "%d-%d/%ld", &tmp1, &tmp2, &size);
                        printf("\rDownloading %s: %.2f %%", filename, tmp2/(float)size*100);
                        fflush(stdout);
                    }
                }

                // download file to client
			    for (len=0; len<bodylen && (n=read(s3, buffer+len, bodylen-len)); len += n);
                fwrite(buffer, bodylen, 1, fout);
			    close(s3);
            }
            printf("\n\n");
            // write to client the downloaded filename
            sprintf(response,"HTTP/1.1 200 Ok\r\nContent-Type:image/jpg\r\nContent-Length:%ld\r\nConnection:close\r\n\r\n", size+85);
            printf("response %d size %ld", strlen(response), size);
            write(s2, response, strlen(response));
            write(s2, buffer, size);
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
