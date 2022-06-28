/*
Pseudocodice:
legge in input una stringa fatta da:
    commando=value&param1=value&param2=value
param1 e param2 possono vuoti, il programma funziona comunque

viene scritto l'intero commando, unendo in un unica stringa command, param1, param2

viene eseguito il comando lanciando system();

viene inserito l'output del comando come entity body della http-response
*/
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main(int argc, char * argv[], char * env[]){
	int i,j,t, c;
	int length;
	char line[500],*key,*value,*buffer;
	char boundary[100];
	char tmp[1024], command[100], mypar1[100], mypar2[100], html[1000], fullCommand[500], response[1000], httpResponse[1000];
	
	// ## TESTING PURPOSES 
	//FILE *txt = fopen("./output.txt", "w+");
	//fprintf(txt, "%s", html);
	//fclose(txt);

	//printf("Ho questo environment:\n");	
	for(i=0;env[i];i++){
		key = env[i];
		for(j=0; env[i][j] != '='; j++);
			env[i][j] = 0;
		value = env[i] + j + 1;
		//printf("key: %s, value: %s\n",key,value);
		if (!strcmp(key, "CONTENT_LENGTH")) 
			length = atoi(value);
	}
	
	// ## INSERTED
	// Struttura enctype="x-www-urlencode"
	// key=value&key=value&key=value

	buffer=malloc(length);
	
	// read key commando=
	int ogLength = length;
	for(i=0;i<length && (t=read(0, tmp+i, strlen("commando=")-i)); i+=t);
	length -= i;
	// Read commando value
	for (i=0; i<length && read(0, command+i, 1); i++) {
		if(command[i] == '&') {
			command[i] = 0;
			break;
		}
	}
	length = length - i - 1;

	// read key param1=
	for(i=0;i<length && (t=read(0,tmp+i, strlen("param1=") -i)); i+=t);
	length -= i;
	// Read param1 value
	for (i=0; i<length && read(0, mypar1+i, 1); i++) {
		if(mypar1[i] == '&') {
			mypar1[i]=0;
			break;
		}
	}
	length = length - i - 1;

	// read key param2=
	for(i=0;i<length && (t=read(0,tmp+i, strlen("param2=") -i)); i+=t);
	length -= i;
	// Read param2 value
	for (i=0; i<length && read(0, mypar2+i, 1); i++);
	mypar2[i]=0;
	length = length - i - 1;

	// execute command and redirect output to file
	sprintf(fullCommand, "%s %s %s > commandOutput.txt", command, mypar1, mypar2);
	system(fullCommand);
	// read redirected output from file and write in response
	FILE * commandOut = fopen("commandOutput.txt", "r");
	for (i=0; (c = fgetc(commandOut)) != EOF; i++)
		response[i] = c;
	response[i] = 0;
	fclose(commandOut);
	
	sprintf(html, "<!DOCTYPE html><html><p>Received command: %s</p><p>Received param1: %s</p><p>Received param2: %s</p><p>Full Command is: %s</p><p>Command output is: %s</p></html>", command, mypar1, mypar2, fullCommand, response);
	sprintf(httpResponse, "HTML/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", strlen(html), html);
	
	// invia la risposta a sw
	for (i=0; (t=write(1, httpResponse+i, strlen(httpResponse)-i)); i+=t);
}
