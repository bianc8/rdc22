/* 
Esame 2015-07-03
Si modifichi il programma che implementa il Web Server in modo che invii un entity body
utilizzando il transfer coding chuncked, come descritto dalla grammatica riportata nella sezione
3.6.1 della RFC 2616, evitando tutte le parti opzionali.

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
    if (-1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
        perror("setsockopt fallita"); return -1;
    }

    // The bind function binds to a specific socket a certain local address/port pair
    if (-1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in))) {
        perror("Bind Fallita"); return -1;
    }

    // Finally, before actually starting the server, the socket needs to be passive, accepting up to 10 remote connections
    if (-1 == listen(s, 10)) {
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
                // Let's terminate the previous request
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
                // Determine the length of the file to be transferred in bytes
                fseek(file, 0L, SEEK_END);
                int size = ftell(file);
                rewind(file);

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