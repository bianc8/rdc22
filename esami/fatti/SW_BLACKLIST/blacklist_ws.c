/*
1 Febbraio 2019

Si modifichi il programma Web Server in modo che si comporti come segue.
Se l'utente di un client richiede una risorsa provenendo da una pagina configurata come “cattiva” (blacklisted),
il Web Server non permette l’accesso alla risorsa richiesta ma ridireziona il client alla pagina di provenienza.
Creare prima il meccanismo e, solo successivamente, gestire un file degli URL in black list.

Si faccia riferimento alla ​RFC 1945
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

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
    char * filename;
    FILE * fin;
    int c;
    int n;
    int i,j,t, s,s2;
    int yes = 1;
    int len;
    if (-1 == ( s = socket(AF_INET, SOCK_STREAM, 0 ))) {
        printf("errno = %d\n",errno); perror("Socket Fallita"); return -1;
    }
    
    local.sin_family = AF_INET;
    local.sin_port = htons(17999);
    local.sin_addr.s_addr = 0;

    t = setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
    if (t==-1) {
        perror("setsockopt fallita"); return 1;
    }

    if (-1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in))) {
        perror("Bind Fallita"); return -1;
    }

    if (-1 == listen(s, 10)) {
        perror("Listen Fallita"); return -1;
    }
    
    remote.sin_family = AF_INET;
    remote.sin_port = htons(0);
    remote.sin_addr.s_addr = 0;

    len = sizeof(struct sockaddr_in);
    
    FILE* blacklist;
    blacklist = fopen("blacklist.txt", "r");
    if (blacklist == NULL) {
        perror("blacklist.txt config file not found"); return -1;
    }

    while (1){
        s2=accept(s,(struct sockaddr *)&remote,&len);
        bzero(hbuffer,10000);
        bzero(h,sizeof(struct header)*100);
        reqline = h[0].n = hbuffer;
        for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
            if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r'){
                hbuffer[i-1]=0; // Termino il token attuale
                if (!h[j].n[0])
                    break;
                h[++j].n = hbuffer+i+1;
            }
            if (hbuffer[i]==':' && !h[j].v){
                hbuffer[i]=0;
                h[j].v = hbuffer + i + 2;
            }
        }
        
        // Blacklist with Referer header
        char* referer = 0;
        int redirect = 0;
        for (i=1; i<j; i++)
            if (!strcmp("Referer", h[i].n)) {
                referer = h[i].v;
                break;
            }



        if (referer) {
            char file_content[100];
            while (fscanf(blacklist, "%[^\n] ", file_content) != EOF)
                if (!strcmp(file_content, referer))
                    redirect = 1;
        }

        if(len == -1) {
            perror("Read Fallita"); return -1;
        }
        
        method = reqline;
        for(i=0;reqline[i]!=' ';i++); reqline[i++]=0; 
        url=reqline+i;
        for(; reqline[i]!=' ';i++); reqline[i++]=0; 
        ver=reqline+i;
        for(; reqline[i]!=0;i++); reqline[i++]=0; 
        
        if (!strcmp(method,"GET")) {
            if (redirect == 1) {
                printf("Filtered referer page: %s\n", referer);
                sprintf(response,"HTTP/1.0 302 Moved Temporarily\r\nLocation:%s\r\n\r\n", referer);
                write(s2,response,strlen(response));
                close(s2);
                continue;
            }
            filename = url+1;
            fin=fopen(filename,"rt");
            if (fin == NULL){
                sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
                write(s2,response,strlen(response));
            } else { 
                sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
                write(s2,response,strlen(response));
                while ( (c = fgetc(fin))!=EOF)
                    write(s2,&c,1);
                fclose(fin);
            }
        }
        else {
            sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
            write(s2,response,strlen(response));
        }
        close(s2);
    }
    fclose(blacklist);
    close(s);
}
