/* 
Esame 2015-07-03
Si modifichi il programma che implementa il web server in modo che questo, 
non appena riceve dal client una request per la risorsa corrispondente al path “/reflect”, 
anziché cercare un file da aprire ed inviare, invii al client una response nella quale l’entity body sia 

a. il testo esatto corrispondente all’intera request inviata dal client al server, comprensiva di tutti gli elementi che la compongono 
b. <CRLF>
c. L’indirizzo IP in notazione decimale puntata da cui il client ha inviato la propria request
d. <CRLF>
e.il port da cui il client ha effettuato la propria request
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

struct sockaddr_in local, remote;

// Create a request/response buffer in the static area so that they are predefined as all zeros
// This should be useful so that the string delimiter is already present (usefulness? not much,
// we change it anyways)
#define buffLen 1024*1024 // 1MB
// This is roughly the same code as the client, but here we are creating two pairs for buffer
// as the server recieves a request, and needs to compose a response.
char request[buffLen];
char response[buffLen];

// Define the header struct in order to save the headers returned in the response
struct Header {
    char *n;
    char *v;
};
// Create an array of headers to save them
struct Header headers[100];

// Declaration of functions
int readUntilNewLine(FILE* file, char* out);
void encodeB64(unsigned char* in, int t, char* out);

int main() {
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Socket fallita con errno = %d\n", errno); perror("Socket fallita"); return -1;
    }

    // Assign the required information to the local address object
    local.sin_family = AF_INET;
    local.sin_port = htons(21667);
    local.sin_addr.s_addr = 0;

    remote.sin_family = AF_INET;
    remote.sin_port = htons(0);
    remote.sin_addr.s_addr = 0;

    int t;
    int yes = 1;
    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
        perror("setsockopt fallita"); return -1;
    }

    // The bind function binds to a specific socket a certain local address/port pair
    if ( bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in)) == -1 ) {
        perror("Bind Fallita"); return -1;
    }

    // Finally, before actually starting the server, the socket needs to be passive, accepting up to 10 remote connections
    if ( listen(s, 10) == -1) {
        perror("Listen fallita"); return -1;
    }

    // This is the main loop of the server, always waiting for new connections to be
    // started from client
    int len = sizeof(struct sockaddr);
    while (1) {
        int rs = accept(s, (struct sockaddr *) &remote, &len);

        // Let's reset the request headers buffer
        bzero(request, buffLen);

        // Clean the headers
        for (int i=0; i<100; i++) {
            headers[i].n = 0;
            headers[i].v = 0;
        }
        int headersCount = 0;

        // The request line is a pointer to the request buffer that contains the HTTP request
        char* reqLine = headers[0].n = request;

        // As we do for the client, let's read the request one character per time
        for (int i=0; read(rs, request+i, 1); i++) {
            // When the termination is reached
            if (request[i] == '\n' && request[i-1] == '\r') {
                // Let's terminate the previous reques (which is at i-1 since the CRLF is 2 chars)
                request[i-1] = 0;
                if (!headers[headersCount].n[0])
                    break;
                // The header key is the string beginning just after the \n char
                headers[++headersCount].n = request + i + 1;
            }
            if (request[i] == ':' && !headers[headersCount].v) {
                // Let's terminate the previous string (the key name)
                request[i] = 0;
                // And place the pointer for the value of the header
                headers[headersCount].v = request + i + 1;
            }
        }

        // Printing the request line
        printf("%s\n", reqLine);
        // And then all the received headers, starting from index 1 as index 0 contains ony the request line
        for (int i=1; i<headersCount; i++)
            printf("%s: %s\n", headers[i].n, headers[i].v);

        //    Request-Line = Methodtruct Header headers[100];
        int p = 0;
        char *method, *url, *version;
        method = reqLine;
        for (p=0; reqLine[p] != ' '; p++);
        reqLine[p++] = 0;
        url = reqLine + p;
        for (; reqLine[p] != ' '; p++);
        reqLine[p++] = 0;
        version = reqLine + p;
        for (; reqLine[p] == '\n' && reqLine[p-1] == '\r'; p++);
        reqLine[p - 1] = 0;

        printf("Method : %s\n", method);
        printf("Url    : %s\n", url);
        printf("Version: %s\n", version);

        // Handing GET requests
        if (!strcmp(method, "GET")) {
            // The file that the client wants is in the url parameter,the first char is '/'
            char* filename = url+1;

            // Try to read the file requested by the client
            FILE* file = fopen(filename, "rt");

            // If no file is found, tell the client that
            if (file == NULL) {
                sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                write(rs, response, strlen(response));
            }
            else {
                // A file in the /secure/ dir, the user must provide header Basic auth
                if (!strncmp(url, "/secure/", strlen("/secure/"))) {
                    // Check if the request comes with an auth header
                    char* authHeader = 0;
                    int i = 0;
                    while (!authHeader && i < headersCount - 1) {
                        if (!strcmp(headers[++i].n, "Authorization"))
                            authHeader = headers[i].v;
                    }

                    printf("Auth found is: %s\n", authHeader);

                    // If the auth header is not provided, return a 401 error and inform the user that
                    // basic authication is available using the 'WWW-Authenticate' header
                    if (!authHeader || strncmp(authHeader, " Basic", strlen(" Basic"))) {
                        sprintf(response, "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\n\r\n");
                        write(rs, response, strlen(response));
                        printf("-- End of request\n\n");

                        fclose(file);
                        close(rs);
                        continue;
                    }

                    // Scan the credentials file and see if the token provided by the client is a valid combination of username:password
                    FILE *credentials = fopen("users.txt", "r");
                    char *token = malloc(100), *encodedToken = malloc(134);
                    int l, userFound = 0;
                    while ((l = readUntilNewLine(credentials, token)) > 0) {
                        // Try to B64 encode each line and compare to the encoded one provided
                        encodeB64((unsigned char*) token, l, encodedToken);
                        // The token starts with " Basic ", so we need to exclude it (add an offset)
                        if (!strcmp(authHeader + strlen(" Basic "), encodedToken)) {
                            userFound = 1;
                            break;
                        }
                    }
                    free(token);
                    free(encodedToken);

                    // If a user is not found by the end of the cycle, this combination of username:password is not valid
                    if (!userFound) {
                        sprintf(response, "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate:basic\r\n\r\n");
                        write(rs, response, strlen(response));
                        printf("-- End of request\n\n");

                        fclose(file);
                        close(rs);
                        continue;
                    }

                }

                // Now that we know the user can ask this resource, determine the length of the file to be transferred in bytes
                fseek(file, 0L, SEEK_END);
                int size = ftell(file);
                rewind(file);

                // If the file is larger than 5KB (arbitrary amount, really), use the chunked transfer encoding
                if (size > (1024 * 5)) {
                    sprintf(response, "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n");
                    write(rs, response, strlen(response));

                    // Create a buffer in which to write the portion read from the file
                    char* fileBuff[1024*5];

                    // Then trying to fill up the buffer, sending the client the amount of the
                    // file read in that cycle, until the whole file has been sent
                    int c=0;
                    for (int l=0; (c = fread(fileBuff, sizeof(char), 1024*5, file)) > 0; l += c) {
                        // Send in the chunk size as hex + CRLF
                        sprintf(response, "%x\r\n", c);
                        write(rs, response, strlen(response));
                        // Followed by the chunk itself
                        write(rs, fileBuff, c);
                        // + CRLF
                        sprintf(response, "\r\n");
                        write(rs, response, strlen(response));
                    }

                    // Sending the last empty chunk per standard
                    sprintf(response, "0\r\n\r\n");
                    write(rs, response, strlen(response));

                    if (!strncmp(url, "/reflect/", strlen("/reflect/"))) {
                        char refl[1024];
                        sprintf(refl, "\n%d.%d.%d.%d\n%d",
                            *((unsigned char*) &remote.sin_addr.s_addr),
                            *((unsigned char*) &remote.sin_addr.s_addr+1),
                            *((unsigned char*) &remote.sin_addr.s_addr+2),
                            *((unsigned char*) &remote.sin_addr.s_addr+3),
                            ntohs(remote.sin_port)
                        );
                        write(rs, refl, strlen(refl));
                    }

                }
                // Otherwise, use the standard transfer method with the Content-Length header
                else {
                    // Which means: send a 200 OK response and include the Content-Length of the response body
                    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length:%d\r\n\r\n", size);
                    write(rs, response, strlen(response));

                    // Then send char by char to the client
                    int c;
                    while(( c = fgetc(file)) != EOF ) write(rs, &c, 1);

                    if (!strncmp(url, "/reflect/", strlen("/reflect/"))) {
                        char refl[1024];
                        sprintf(refl, "\n%d.%d.%d.%d\n%d",
                            *((unsigned char*) &remote.sin_addr.s_addr),
                            *((unsigned char*) &remote.sin_addr.s_addr+1),
                            *((unsigned char*) &remote.sin_addr.s_addr+2),
                            *((unsigned char*) &remote.sin_addr.s_addr+3),
                            ntohs(remote.sin_port)
                        );
                        write(rs, refl, strlen(refl));
                    }
                }
                fclose(file);
            }

        }
        // When the method is not one of the implemented one, send the client a 501 HTTP response
        else {
            sprintf(response, "HTTP/1.1 501 Not Implemented\r\n\r\n");
            write(rs, response, strlen(response));
        }

        // And close the connection socket, to signal that the data is over.
        close(rs);
        printf("-- End of request\n\n");
    }
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


/**
 * encodeB64 allows to encode a
 * char* in   string to encode
 * int   t    lenght of the in string
 * char* out  B64 result
 */
void encodeB64(unsigned char* in, int t, char* out) {
    char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Every 3 unsigned int (1Byte variables) make up 4 B64 chars
    // Using a loop, I will take groups of 3 chars to then converted
    int l = 0;
    int e = 0;

    for (int i = 0; l < t; i = ++i%3, l++) {
        // If we are reading the first character we need to remove the last
        // 2 characters
        if (i==0){
            out[e++] = b64[in[l] >> 2];
            out[e++] = b64[(in[l] & 0x03) << 4];
        }
        // If we are reading the second char we need to construct the second and third B64
        // chars
        if (i==1) {
            out[e - 1] = b64[((in[l - 1] & 0x03) << 4) | (in[l] >> 4)];
            out[e++] = b64[(in[l] & 0x0F) << 2];
        }
        // Finally, we are at the third char, let's complete the 3rd B64 char
        // and create the 4th
        if (i==2) {
            out[e - 1] = b64[((in[l - 1] & 0x0F) << 2) | (in[l] >> 6)];
            out[e++]   = b64[in[l] & 0x3F];
        }

    }
    if (l%3 == 2 || l%3 == 1)
        out[e++] = '=';
    if (l%3 == 1) out[e++] = '=';
        out[e] = 0;

}
