/**
se il client richiede l'accesso di una risorsa presente nella carella /secure (GET /secure/prova.html)
il server richiederà l'autenticazione
e controllerà se username e password sono in un file di testo organizzato a righe: <username>SP<password>CR

Se l'utente viene autenticato verrà inviata la risorsa richiesta, altrimenti verrà nuovamente richiesta l'autenticazione
**/
#include<stdlib.h>
#include<stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>

char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// Create a reqLine/response buffer in the static area (all zeros)
#define buffLen 1024*1024 // 1MB

struct sockaddr_in local, remote;

char reqLine[buffLen];
char hRequest[buffLen];
char response[buffLen];

// Define the header struct in order to save the headers returned in the response
struct Header {
   char *n;
   char *v;
};
// Create an array of headers to save them
struct Header headers[100];

void f(unsigned char *a, char *d){
	d[0] = base64[a[0] >> 2];
	d[1] = base64[((a[0] << 4) & 0x30) | (a[1] >> 4)];
	d[2] = base64[((a[1] << 2) & 0x3C) | (a[2] >> 6)];
	d[3] = base64[a[2] & 0x3F];
}

void base64_encode(unsigned char *in, int in_len, char *out){
	int out_offset = 0;
	int i;
	for(i=0; i<in_len-2; i+=3){
		f(in + i, out + out_offset);
		out_offset += 4;
	}
	if(in_len%3==2){
		unsigned char x[3];
		x[0] = in[i];
		x[1] = in[i+1];
		x[2] = 0;
		f(x, out + out_offset);
		out[out_offset + 3] = '=';
	}
	else if(in_len%3==1){
		unsigned char x[3];
		x[0] = in[i];
		x[1] = 0;
		x[2] = 0;
		f(x, out + out_offset);
		out[out_offset + 2] = out[out_offset + 3] = '='; 
	}
	out_offset += 4;
	out[out_offset] = '\0';
}

/**
 * readUntilNewLine reads from the file pointed by file and
 * puts in the out buffer the characters until a \n char is reached
 */
int readUntilNewLine(FILE* file, char* out) {
   int i = 0;
   for (char c; (c = fgetc(file)) != EOF && c != '\n'; i++) out[i] = c;
   out[i + 1] = 0;

   return i;
}

/*
 * matchUser reads from credentials file and check if user:passwords exists
*/
int matchUser( char *user) {
    FILE *credentials = fopen("passwd.txt", "r");
    if (credentials == NULL) {
        perror("File not found");
        return -1;
    }
    char *line = malloc(100), *encodedLine = malloc(134);
    int n, userFound = -1;
    while ((n = readUntilNewLine(credentials, line)) > 0) {
        // Try to B64 encode each line and compare to the encoded one provided
        printf("line %s\n", line);
        base64_encode((unsigned char*) line, n, encodedLine);
        if (strcmp(user, encodedLine) == 0) {
            userFound = 1;
            break;
        }
    }
    free(line);
    free(encodedLine);
    return userFound;
}

int main() {  
    int s;
    if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0))) {
        printf("errno = %d\n", errno); perror("Socket Fallita"); return -1;
    }
    local.sin_family = AF_INET;     // sa_family_t address family
    local.sin_port = htons(17999);  // in_port_t port in network byte order (16 bit)
    local.sin_addr.s_addr = 0;      // struct_in_addr internet address { uint32_t s_addr } aKa address in network byte order

    remote.sin_family = AF_INET;
    remote.sin_port = htons(0);
    remote.sin_addr.s_addr = 0;

    int yes = 1;
    if (-1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))){
        perror("setsockopt fallita"); return 1;
    }

    if (-1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in))){
        perror("Bind Fallita"); return -1;
    }

    // int listen(int sockfd, int backlog = 10 = max queue length)
    if (-1 == listen(s, 10)) {
        perror("Listen Fallita"); return -1;
    }
    
    int len = sizeof(struct sockaddr_in);
    while (1){
        // int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen, int flags)
        int s2 = accept(s, (struct sockaddr *)&remote, &len);
        bzero(hRequest, buffLen);

        // Clean the headers
        for (int i=0; i<100; i++) {
            headers[i].n = 0;
            headers[i].v = 0;
        }
        int headersCount = 0;

        // The reqLine line is a pointer to the reqLine buffer that contains the HTTP reqLine
        char* reqLine = headers[0].n = hRequest;

        // As we do for the client, let's read the reqLine one character at a time until there
        // is something to read. When a termination char is reached, the loop will be broken
        for (int i=0; read(s2, hRequest+i, 1); i++) {
            // reached the end of one header
            if (hRequest[i] == '\n' && hRequest[i-1] == '\r') {
                hRequest[i-1] = 0;

                // if the last read header is all zeros, we finished all the headers
                if (!headers[headersCount].n[0]) break;

                // The value is the string beginning just after the \n char
                headers[++headersCount].n = hRequest + i + 1;
            }

            // When a ":" is reached, there is a value for the just read key
            if (hRequest[i] == ':' && !headers[headersCount].v) {
                hRequest[i] = 0;    // End the key
                // And place the pointer for the value string
                headers[headersCount].v = hRequest + i + 1;
            }
        }

        printf("%s\n", reqLine);    // print reqLine line
        for (int i=1; i<headersCount; i++)  // print headers
            printf("%s: %s\n", headers[i].n, headers[i].v);

        char * method, *url, *ver;  
        method = reqLine;   // consumo e estraggo i token senza copiare byte
        
        int p = 0;  // p act as offset
        for(p = 0; reqLine[p] != ' '; p++);
        reqLine[p++] = 0; 
        url = reqLine + p;
        
        for( ; reqLine[p]!=' '; p++);
        reqLine[p++] = 0; 
        ver = reqLine + p;

        for( ; reqLine[p] =='\n' && reqLine[p-1] == '\r'; p++);
        reqLine[p-1] = 0;
        
        if (!strcmp(method, "GET")){
            char * filename = url + 1; // senza prendere il primo slash, x es: /index.html
            
            // try to read requested file
            FILE * fin = fopen(filename, "rt");
            
            // null => no file found
            if (fin == NULL){
                sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                write(s2, response, strlen(response));
            }
            // file found, write it in response
            else{
                if (!strncmp(url, "/secure/", strlen("/secure/"))) {
                    // Check if the request comes with an auth header
                    char* authHeader = 0;
                    int i = 0;
                    while (!authHeader && i < headersCount - 1) {
                        if (strcmp(headers[++i].n, "Authorization") == 0)
                            authHeader = headers[i].v;
                    }

                    printf("Auth found is: %s\n", authHeader);

                    // If the auth header is not provided, return a 401 error and inform the user that
                    // basic authication is available using the 'WWW-Authenticate' header
                    // This same type of response is given if the auth token is not a Bearer token
                    if (!authHeader || (strncmp(authHeader, " Basic", strlen(" Basic")) != 0)) {
                        sprintf(response, "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\n\r\n");
                        write(s2, response, strlen(response));
                        printf("-- End of request\n\n");
                        fclose(fin);
                        close(s2);
                        continue;
                    }
                    
                    int userFound = -1 < matchUser(authHeader+strlen(" Basic "));

                    // If a user is not found by the end of the cycle, this combination of username
                    // and password is not valid and the request is unauthorized
                    if (!userFound) {
                        sprintf(response, "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\n\r\n");
                        write(s2, response, strlen(response));
                        printf("-- End of request\n\n");
                        fclose(fin);
                        close(s2);
                        continue;
                    }
                }
                // Now that we know the user can ask this resource, determine the length of the
                // file to be transferred in bytes
                fseek(fin, 0L, SEEK_END);
                int size = ftell(fin);
                rewind(fin);
                // if file is bigger than 5 KB
                if (size > (1024 * 5)) {
                    sprintf(response, "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n");
                    write(s2, response, strlen(response));

                    // Create a buffer in which to write the portion read from the file
                    char* fileBuff[1024*5]; // 5 KB

                    // Try to fill up the buffer, sending the client the chunks, until the whole file has been sent
                    int c=0;
                    for (int l=0; (c = fread(fileBuff, sizeof(char), 1024*5, fin)) > 0; l += c) {
                        // Send in the chunk size as hex + CRLF
                        sprintf(response, "%x\r\n", c);
                        write(s2, response, strlen(response));
                        // Followed by the chunk itself
                        write(s2, fileBuff, c);
                        // + CRLF
                        sprintf(response, "\r\n");
                        write(s2, response, strlen(response));
                    }

                    // Sending the last empty chunk per standard
                    sprintf(response, "0\r\n\r\n");
                    write(s2, response, strlen(response));

                }
                // Otherwise, use the standard transfer method with the Content-Length header
                else {
                    // Which means: send a 200 OK response and include the Content-Length of the response body
                    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length:%d\r\n\r\n", size);
                    write(s2, response, strlen(response));

                    // Then send each char to the client
                    int c;
                    while(( c = fgetc(fin)) != EOF ) write(s2, &c, 1);
                }
                fclose(fin);
            }
        }
        else {
            sprintf(response, "HTTP/1.1 501 Not Implemented\r\n\r\n");
            write(s2, response, strlen(response));
        }
        close(s2);
    }
    close(s);
}