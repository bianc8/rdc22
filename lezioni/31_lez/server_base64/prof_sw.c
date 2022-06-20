#include<stdlib.h>
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

int main()
{
char * method, *url, *ver;
char * filename;
char command[100];
FILE * fin;
int c;
int n;
int i,t, s,s2;
int yes = 1;
int len;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
local.sin_family = AF_INET;
local.sin_port = htons(17999);
local.sin_addr.s_addr = 0;

t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
if (t == -1){perror("Setsockopt fallita"); return 1;}

if ( -1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in)))
{ perror("Bind Fallita"); return -1;}

if ( -1 == listen(s,10)) { perror("Listen Fallita"); return -1;}
remote.sin_family = AF_INET;
remote.sin_port = htons(0);
remote.sin_addr.s_addr = 0;
len = sizeof(struct sockaddr_in);
while ( 1 ){
	bzero
	s2=accept(s,(struct sockaddr *)&remote,&len);
	len = read(s2,request,1000);
	request[len]=0;
	printf("%s",request);
	if(len == -1) { perror("Read Fallita"); return -1;}
	method = request;
	for(i=0;i<len && request[i]!=' ';i++); request[i++]=0; 
	url=request+i;
	for(;i<len && request[i]!=' ';i++); request[i++]=0; 
	ver=request+i;
	for(;i<len && request[i]!='\r';i++); request[i++]=0; 
	if ( !strcmp(method,"GET")){
		filename = url+1;
		if(!strncmp(url,"/cgi-bin/",strlen("/cgi-bin/"))){
				sprintf(command,"%s > tmpfile",(url+strlen("/cgi-bin/")));			
				printf("Eseguo comando %s\n", command);
				system(command);
				strcpy(filename,"tmpfile");
				}	
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
	else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
	}
	close(s2);
}
close(s);
}
