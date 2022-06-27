# Important RFC

[RFC 1945 HTTP/1.0](https://datatracker.ietf.org/doc/html/rfc1945)

[RFC 2616 HTTP/1.1](https://datatracker.ietf.org/doc/html/rfc2616)

# Functions

Here there are some useful functions

# Print Client Ip and Port to string

<details>
<summary>Click to expand!</summary>
    
```c
    struct sockaddr_in remote;
    printf("Client ip: %d.%d.%d.%d \nPort: %d",
        *((unsigned char*) &remote.sin_addr.s_addr),
        *((unsigned char*) &remote.sin_addr.s_addr+1),
        *((unsigned char*) &remote.sin_addr.s_addr+2),
        *((unsigned char*) &remote.sin_addr.s_addr+3),
        ntohs(remote.sin_port)
    );
```
</details>

# gethostbyname()

<details>
<summary>Click to expand!</summary>
    
```c
    /**
    struct hostent {
        char  *h_name;            // official name of host
        char **h_aliases;         // alias list
        int    h_addrtype;        // host address type
        int    h_length;          // length of address
        char **h_addr_list;       // list of addresses
    }
    #define h_addr h_addr_list[0] // for backward compatibility
    */
    
    char hostname[1000];
    sprintf(hostname, "www.example.com");
    struct hostent *remoteIp;
    remoteIp = gethostbyname(hostname);

    printf("Indirizzo di %s : %d.%d.%d.%d\n", hostname,
        (unsigned char)(remoteIp->h_addr[0]), (unsigned char)(remoteIp->h_addr[1]),
        (unsigned char)(remoteIp->h_addr[2]), (unsigned char)(remoteIp->h_addr[3]));
```
</details>

# HTTP Date

An example of HTTP Date

-   Thu, 17 Oct 2019 07:18:26 GMT

So HTTP date format is

-   %a, %d %b %Y %H:%M:%S %Z

Here are some functions for handling HTTP Dates:

## Save the actual HTTP Date in date_buf array of char    
    
<details>
<summary>Click to expand!</summary>  
    
```c
    char date_buf[1000];

    char* getNowHttpDate(){
        char* format = "%a, %d %b %Y %H:%M:%S %Z";
        time_t now = time(0);
        struct tm tm = *gmtime(&now);
        strftime(date_buf, sizeof(date_buf), format, &tm);
        printf("Time is: [%s]\n", date_buf);
        return date_buf;
    }
```
</details>


## Save the actual epoch in the cached file

<details>
<summary>Click to expand!</summary>
    
```c
    void saveEpoch() {
        FILE* cached = fopen(cacheName, "w+");
        if (cached != NULL) {
            time_t epochNow = time(NULL);
            fprintf(cached, "%lu\n", (unsigned long)epochNow);
        }
        fclose(cached);
    }
```
</details>
    

## Parse HTTP Date and convert it to ms since the epoch

<details>
<summary>Click to expand!</summary>
    
```c
    time_t httpDateToEpoch(char * lastModified) {    
        char* format = "%a, %d %b %Y %H:%M:%S %Z";
        struct tm* httpTime = malloc(sizeof(struct tm));
        strptime(lastModified, format, httpTime);    // man 3 strptime
        time_t epochRemote = mktime(httpTime);  // convert tm to epoch
        return epochRemote;
    }
```
</details>

## Check if the httpDate saved in the cached file is expired

<details>
<summary>Click to expand!</summary>

```c
    /**
    * Replace char '/' with '_' in the given string
    */
    void charReplace(char* s) {
        for (int i=0; s[i]; i++)
            if (s[i] == '/')
                s[i] = '_';
    }

    unsigned char expired(char * uri, char * last_modified){
        char resourceName[] = "/";
        
        // Replace '/' with '_' in resourceName
        char tmp[100];
        strcpy(tmp, resourceName);
        charReplace(tmp);
        
        // Save cacheName
        char cacheName[1000];
        strcpy(cacheName, "./cache/");
        strcat(cacheName, tmp);

        // Open cacheName file
        FILE * cached = fopen(cacheName, "r");
        if (cached == NULL)
            return 1;
        
        // read first line of cached file
        char * line = 0; size_t len = 0;
        getline(&line, &len, cached);
        
        if (httpDateToEpoch(last_modified) < httpDateToEpoch(line))
            return 0;
        return 1;
    }
```
</details>

# Read a file and send it (SW)

<details>
<summary>Click to expand!</summary>

```c
    FILE *fin;
    fin = fopen(uri + 1, "rt"));
    if (fin == NULL) {
        sprintf(response, "HTTP/1.1 404 File not found\r\n\r\n<html>File non trovato</html>");
        t = write(s2, response, strlen(response));
        if (t == -1) {
            perror("write fallita"); return -1;
        }
    } else {
        content_length = 0;
        while ((c = fgetc(fin)) != EOF) content_length++; // get file length
        sprintf(response, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n", content_length);
        printf("Response: %s\n", response);
        //send headers
        t = write(s2, response, strlen(response));
        if (t==-1) {
            perror("write fallita"); return -1;
        }
        rewind(fin);    // move pointer to the begin of the file
        //re-read and send the file, char per char
        while ((c = fgetc(fin)) != EOF) {
            if (write(s2, (unsigned char *)&c, 1) != 1)
                perror("Write fallita");
        }
        fclose(fin);
    }
```
</details>

# Useful tips

Useful info

**/etc/services**: To know all the TCP ports available at the application level.

**/etc/protocols**. Know assigned Internet Protocol Numbers. In the IPv4 there is an 8 bit field "Protocol" to identify the next level protocol.In IPv6 this field is called the "Next Header" field.

**nslookup <URL>**: finds the ip address of the specified URL (example: www.google.com)

**netstat -rn** shows routing table

**traceroute <URL>** routes an ip packet in which path it travels by printing the IP of every gateway that decides to drop the packet that was forged with low TTL (time to live, decremented on every hop) count.


# How to use Vim and other tips

## Preconditions
**Copy the .vimrc file into your home directory on the server using Unix's scp command**.

Remember to change MATRICOLA with your Unipd Student ID.

Login using your SSH credentials.

    scp -O ./.vimrc MATRICOLA@88.80.187.84:/home/MATRICOLA/.vimrc 


## Content of .vimrc
This .vimrc config file allows you to:
- **Use your mouse**: scrolling wheel to move up or down, click and point
- **Move line Up/Down** using Ctrl+Shift+Up/Down
- Press F8 to **Compile** the program **without exiting Vim**
- Press F9 to **Execute** the program **without exiting Vim**

## Other configurations:
- Replace tabs with 3 spaces
- Highlight matching parentheses
- Auto indent on brackets
- Show line number
- Highlight current line
- Search characters as they are entered
- Search is case insesitive if no case letters are entered, but case sensitive if case letters are entered
- Highlight search results


# How to search in VIM:
<details>
<summary>Click to expand!</summary>

<br>

Search is **UNIDIRECTIONAL** but when the search reach one end of the file, pressing **n** continues the search, starting from the other end of the file.

## Search from the current line **forward**/**backwards**

To search forward use /

To search bacward use ?

x es:

    ESC (go into Command mode)

    /query (forward)
    ?query (backward)

    ENTER (to stop writing in the search query)

    (now all search results of the query are highlighted)

    n (to move to the NEXT occurence in the search results)
    N (to move to the PREVIOUS occurence in the search results)

    ESC (to exit Search mode)
</details>


# How to Compile and Execute without exiting VIM:
<details>
<summary>Click to expand!</summary>

To Compile press F8

To Execute press F9

    ESC (go into Command mode)

    F8 (compile shortcut)
    F9 (execute shortcut)

    CTRL+C (to exit compilation/executable) 

    Enter (to re-enter in vim)
</details>



# How to Move current line Up or Down in VIM:
<details>
<summary>Click to expand!</summary>

    ESC (go into Command mode)

    CTRL+SHIFT+PAGE UP  (to move line up)
    CTRL+SHIFT+PAGE DOWN (to move line down)

    i (go into Insert mode)
</details>


# How to Select, Copy/Cut and Paste in VIM:
<details>
<summary>Click to expand!</summary>

    Select with the mouse the text you want to copy
    [ALTERNATIVE
        ESC (go into Command mode)
        V100G (to select from current line to line 100, included, using Visual mode)]

    y (to Copy/yank)
    d (to Cut/delete)

    p (to Paste after the cursor)
</details>

# How to copy from another file in VIM:
<details>
<summary>Click to expand!</summary>

Open the file from which you want to copy in Vim using:

    vi ogFile.c (ogFile is the destination file)

    ESC (go into Command mode)

    :ePATH/file (open 'source' file at Path)

    (select the lines that you want to copy)
    y (copy/yank)

    :q (close the 'source' file)

    vi ogFile.c (open the 'destination' file)

    p (paste the copied lines into the 'destination' file)
</details>



# If you've made an error, CTRL+z is u:
<details>
<summary>Click to expand!</summary>
    
    ESC (go into Command mode)

    u (to Undo)
</details>


# If you've pressed CTRL + s and now the screen is frozen, press CTRL + q (to unfreeze screen)
<details>
<summary>Click to expand!</summary>

    CTRL + s (now screen is frozen)

    (every command that you type when the screen is frozen will be executed, it just won't be displayed in the terminal)

    CTRL + q (to unfreeze the screen)
</details>
