/*
Esame di Reti di Calcolatori - 20 Giugno 2018

Si modifichi il programma ws18.c in modo tale che per accedere a qualunque contenuto all’utente sia richiesta
l’immissione di una login e una password tramite protocollo HTTP.
La login dev’essere il proprio nome di battesimo e la password il proprio numero di matricola
Si provi il programma con il browser del PC locale.
Si faccia riferimento alla RFC 1945 per il metodo di autenticazione basic.



se il client richiede l'accesso di una risorsa presente nella carella /secure (GET /secure/prova.html)
il server richiederà l'autenticazione
e controllerà se username e password sono in un file di testo organizzato a righe: <username>SP<password>CR

Se l'utente viene autenticato verrà inviata la risorsa richiesta, altrimenti verrà nuovamente richiesta l'autenticazione
*/
#include<stdlib.h>
#include<stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>

// base64 alphabet
char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// Create a reqLine/response buffer in the static area (all zeros)
#define buffLen 1024*1024 // 1MB
// Define the header struct in order to save the headers returned in the response
struct Header {
   char *n;
   char *v;
};


struct sockaddr_in local, remote;
char reqLine[buffLen];
char hRequest[buffLen];
char response[buffLen];

// Create an array of headers to save them
struct Header headers[100];


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
	out[out_offset] = '\0';
}

/** readUntilNewLine reads from the file and puts in the out buffer the characters until a \n char is reached
 * 
 * @param   FILE* file  file to read line by line
 * @param   char* out   string to put line read
 * 
 */
int readUntilNewLine(FILE* file, char* out) {
   int i = 0;
   for (char c; (c = fgetc(file)) != EOF && c != '\n'; i++) out[i] = c;
   out[i + 1] = 0;

   return i;
}

/** matchUser reads from credentials file and check if user:password exists
 * 
 * @param   char *user  is the token passed via Basic Authentication to see if it exists
 * 
*/
int matchUser( char *user) {
    FILE *file = fopen("passwd.txt", "r");
    if (file == NULL) {
        perror("File not found"); return -1;
    }
    // read file line by line
    char *line = malloc(100), *encodedLine = malloc(134);
    int n, userFound = -1;

    while ((n = readUntilNewLine(file, line)) > 0) {
        // Try to encode in Base64 each line and compare it to the encoded user provided in Auth header
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

    int yes = 1, userFound = 0;
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

        // reqLine line is a pointer to the reqLine buffer that contains the HTTP request line
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
                hRequest[i] = 0;    // End the key of the header
                headers[headersCount].v = hRequest + i + 1; // place the pointer for the value string
            }
        }

        printf("%s\n", reqLine);    // print the request line
        for (int i=1; i<headersCount; i++)  // print all the headers headers[0] = reqLine
            printf("%s: %s\n", headers[i].n, headers[i].v);

        // read method, url and version
        char * method, *url, *ver;  
        int p = 0;  // p act as offset
        // read method
        method = reqLine;
        // read url
        for(p = 0; reqLine[p] != ' '; p++);
        reqLine[p++] = 0; 
        url = reqLine + p;
        // read version
        for( ; reqLine[p]!=' '; p++);
        reqLine[p++] = 0; 
        ver = reqLine + p;
        // end request line
        for( ; reqLine[p] =='\n' && reqLine[p-1] == '\r'; p++);
        reqLine[p-1] = 0;
        
        // handle GET request
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
                // if requested file is in the secure/ directory, check auth header
                if (!strncmp(url, "/secure/", strlen("/secure/"))) {
                    char* authHeader = 0;
                    int i = 0;
                    while (!authHeader && i < headersCount - 1) {
                        // read authorization header
                        if (!strcmp(headers[++i].n, "Authorization")) {
                            authHeader = headers[i].v;
                        }
                    }

                    printf("Auth found is: %s\n", authHeader);

                    if (authHeader)
                        userFound = -1 < matchUser(authHeader+strlen("Basic "));
                    // If the auth header is not provided, return a 401 error and inform the user that
                    // basic authication is available using the 'WWW-Authenticate' header
                    // This same type of response is given if the auth token is not a Bearer token
                    // If a user is not found by the end of the cycle, this combination of username
                    // and password is not valid and the request is unauthorized
                    if (!authHeader || strncmp(authHeader, " Basic", strlen(" Basic")) || !userFound) {
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