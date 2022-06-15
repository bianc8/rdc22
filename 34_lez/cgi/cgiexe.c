#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>


char *replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    len_rep = strlen(rep);
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count)
        ins = tmp + len_rep;

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    // first time through the loop, all the variable are set correctly from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

int main(int argc, char * argv[], char * env[]){
	FILE* txt;
	int i,j,t;
	int length;
	char line[500],*key,*value,*buffer;
	char boundary[100];
	char tmp[1024], str1[30], str2[30];
	//fgets(line,500,stdin);
	
	//printf("Io sono il cgiexe e ho letto questa riga da stdin: %s",line);
	printf("E ho questo environment:\n");	
	for(i=0;env[i];i++){
		key = env[i];
		for(j=0;env[i][j]!='=';j++);
			env[i][j]=0;
		value = env[i]+j+1;
		printf("key:%s, value:%s\n",key,value);	
		if (!strcmp(key,"CONTENT_LENGTH")) 
			length = atoi(value);
		if (!strcmp(key,"BOUNDARY")) 
			sprintf(boundary, value);
	}
	// INSERTED
	// Struttura enctype="multipart/form-data"
	// --boundary
	// Content-Disposition: form-data; name={NAME} (; filename={FILENAME} | NULL)
	// (Content-Type: "text/html" | NULL)
	// CLRF
	// {VALUE}
	// se Content-Disposition == NULL ==> EOF
	buffer=malloc(length);
	
	// read --boundary + CRLF
	for(i=0;i<length && (t=read(0,tmp+i, strlen(boundary)+4 -i)); i+=t);
	length = length - strlen(boundary) - 4;
	// read content-disposition+CLRF+CRLF
	for(i=0;i<length && (t=read(0, tmp+i, 43+4-i)); i+=t);
	length = length - 43 - 4;
	// Read str1
	for (i=0,j=0; read(0, str1+i, 1); i++) {
		if(str1[i] == '\n' && str1[i-1]=='\r') {
			str1[i-1]=0;
			break;
		}
	}
	length = length - i - 2;
	// read --boundary + CRLF
	for(i=0;i<length && (t=read(0,tmp+i, strlen(boundary)+4 -i)); i+=t);
	length = length - strlen(boundary) - 4;
	// read content-disposition+CLRF+CRLF
	for(i=0;i<length && (t=read(0, tmp+i, 43+4-i)); i+=t);
	length = length - 43 - 4;
	// Read str2
	for (i=0,j=0; read(0, str2+i, 1); i++) {
		if(str2[i] == '\n' && str2[i-1]=='\r') {
			str2[i-1]=0;
			break;
		}
	}
	length = length - i - 2;
	// read --boundary + CRLF
	for(i=0;i<length && (t=read(0,tmp+i, strlen(boundary)+4 -i)); i+=t);
	length = length - strlen(boundary) - 4;
	// read content-disposition+CLRF+CRLF
	for (i=0,j=0; read(0, tmp+i, 1); i++)
		if(tmp[i] == '\n' && tmp[i-1]=='\r')
			break;
	length = length - i - 2;
	// read content-type 23 chars
	for(i=0;i<length && (t=read(0, tmp+i, 23+4-i)); i+=t);
	length = length - 23 - 4;
	// read file
	length = length - strlen(boundary) - 4;
	for(i=0;i<length && (t=read(0,buffer+i,length-i));i+=t);
	char *final = replace(buffer, str1, str2);
	
    // save the request in the file
    txt = fopen("prova.txt", "w+");
    fwrite(final, sizeof(final[0]), strlen(final), txt);

	// invia la risposta a sw
	length = strlen(final);
	for(i=0;i<length && (t=write(1, final+i,length-i)); i+=t);

	printf("Ciao, muoio\n");
}
