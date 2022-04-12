#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

FILE * f;

int main() {
   int s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 3

   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 4
   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 5
   
   // socket restituisce un indice
   // evidentemente 0, 1, 2 sono già usati
   // 3 file descriptor già in uso: stdinput, stdoutput, stderror

   f = fopen("file.txt", "wt");  // 6

   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 7
   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 8

   fclose(f);

   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); //6 (liberato il file)
   s = socket(AF_INET, SOCK_STREAM, 0);
   printf("s = %d\n", s); // 9 (7 e 8 sono in uso)


   // IL CLIENT PRENDE INIZIATIVA
   // Connessione in rete è l' apertura di uno stream 
   
   // Chiusura connessione è la chiusura dello stream

   // get


   return 0;
}