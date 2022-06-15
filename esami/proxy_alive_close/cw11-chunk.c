#include<stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

struct sockaddr_in remote;
char response[1000001];
struct header {
	char * n;
	char * v;
} h[100];

int main()
{
size_t len = 0;
int i,j,k,n,offset,chunklen,bodylen=0;
char * request = "GET / HTTP/1.1\r\nHost:www.google.it\r\n\r\n";
char * statusline;
char hbuffer[10000], ckbuffer[100];
unsigned char ipserver[4] = { 142,250,180,3};
int s;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
remote.sin_family = AF_INET;
remote.sin_port = htons(80);
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
	bodylen=0;
	for(i=1;i<j;i++){
		printf("%s ---> %s\n",h[i].n,h[i].v);
		if(!strcmp("Content-Length",h[i].n))
			bodylen=atoi(h[i].v);
		if(!strcmp("Transfer-Encoding",h[i].n))
			if (!strcmp(" chunked",h[i].v)) bodylen = -1;	
	}
	if(bodylen == -1){
	offset = 0;
	chunklen = 1;
	while(chunklen){
		chunklen = 0;
		for(j=0; (n = read(s, ckbuffer + j, 1)) > 0 && ckbuffer[j] != '\n' && ckbuffer[j-1] != '\r'; j++){
			if(ckbuffer[j] >= 'A' && ckbuffer[j] <= 'F')
				chunklen = chunklen*16 + (ckbuffer[j]-'A'+10);
			if(ckbuffer[j] >= 'a' && ckbuffer[j] <= 'f')
				chunklen = chunklen*16 + (ckbuffer[j]-'a'+10);
			if(ckbuffer[j] >= '0' && ckbuffer[j] <= '9')
				chunklen = chunklen*16 + ckbuffer[j]-'0';
		}
		if (n==-1) { perror("Read fallita"); return -1;}
		ckbuffer[j-1] = 0;
		printf("chunklen ---> %d\n", chunklen);
		printf("ckbuffer ---> %s\n", ckbuffer);
	for(len=0; len<chunklen && (n = read(s,response + offset, chunklen - len))>0; len+=n, offset+=n);
	if (n==-1) { perror("Read fallita"); return -1;}
	read(s, ckbuffer, 1);
	if (n==-1) { perror("Read fallita"); return -1;}
	read(s, ckbuffer +1,1);
	if (n==-1) { perror("Read fallita"); return -1;}
	if(ckbuffer[0]!='\r' || ckbuffer[1] != '\n'){ printf("Errore nel chunk"); return -1;}
	}
	response[offset] = 0;
	printf("%s\n", response);
}
}
}
/*
3.6.1 Chunked Transfer Coding

   The chunked encoding modifies the body of a message in order to
   transfer it as a series of chunks, each with its own size indicator,
   followed by an OPTIONAL trailer containing entity-header fields. This
   allows dynamically produced content to be transferred along with the
   information necessary for the recipient to verify that it has
   received the full message.

       Chunked-Body   = *chunk
                        last-chunk
                        trailer
                        CRLF

       chunk          = chunk-size [ chunk-extension ] CRLF
                        chunk-data CRLF
       chunk-size     = 1*HEX
       last-chunk     = 1*("0") [ chunk-extension ] CRLF

       chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
       chunk-ext-name = token
       chunk-ext-val  = token | quoted-string
       chunk-data     = chunk-size(OCTET)
       trailer        = *(entity-header CRLF)

   The chunk-size field is a string of hex digits indicating the size of
   the chunk. The chunked encoding is ended by any chunk whose size is
   zero, followed by the trailer, which is terminated by an empty line.

   The trailer allows the sender to include additional HTTP header
   fields at the end of the message. The Trailer header field can be
   used to indicate which header fields are included in a trailer (see
   section 14.40).*/		
