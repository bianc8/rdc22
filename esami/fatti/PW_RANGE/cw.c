#include <stdio.h>
#include <string.h>  //strlen
#include <sys/types.h>  // ""
#include <sys/socket.h> // socket, struct sockaddr
#include <errno.h>
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <stdlib.h>
#include <netdb.h>  // gethostbyname

struct sockaddr_in remote;
char response[1000001];
struct header {
	char * n;
	char * v;
} h[100];

int main()
{
size_t len = 0;
int i,j,k,n,bodylen;

char hostname[1000];
sprintf(hostname, "88.80.187.84");

// Resolving the hostname to an IP address
//printf("Resolving IP for %s...\n", hostname);
struct hostent* remoteIP = gethostbyname(hostname);

char resourceName[100];
sprintf(resourceName, "/immagine.jpg");
FILE * fout = fopen(resourceName+1, "wb");

char request[10];
sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", resourceName, hostname);

char * statusline;
char hbuffer[10000];
unsigned char ipserver[4] = { 88, 80, 187, 84};

int s;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }

remote.sin_family = AF_INET;
remote.sin_port = htons(17999);
remote.sin_addr.s_addr = *((uint32_t *) ipserver);

if ( -1 == connect(s, (struct sockaddr *)&remote,sizeof(struct sockaddr_in)))
{ perror("Connect Fallita"); return -1;}

for(k=0; k < 1; k++){
	write(s,request,strlen(request));
	bzero(hbuffer,10000);
	statusline = h[0].n = hbuffer;
	for (i=0,j=0; read(s,hbuffer+i,1); i++) {
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
	bodylen=1000000;
	for(i=1;i<j;i++){
		printf("%s ---> %s\n",h[i].n,h[i].v);
		if(!strcmp("Content-Length",h[i].n))
			bodylen=atoi(h[i].v);
	}
	for(len=0; len<bodylen && (n = read(s,response + len,1000000 - len))>0; len+=n);
	if (n==-1) { perror("Read fallita"); return -1;}
	response[len]=0;
	fwrite(response, len, 1, fout);
}

fclose(fout);
close(s);
return 0;
}		
