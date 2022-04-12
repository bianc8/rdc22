#include<stdio.h>
#include <stdlib.h>

// var in memoria statica
int a; 

// segmentation fault, il sistema salva segmenti si cerca di accedere a un area di memoria di cui il SO non associa al processo
// segmentation fault succede anche se si finisce memoria fisica

// var dinamiche che occupano memoria se vengono allocate con malloc

int fattoriale(int x) {
   //malloc va in heap (crescente)
   int *p = (int*)malloc(sizeof(int));
   // params di funzione vanno in stack (decrescente)
   // stack overflow: stack overwrite heap che cresce in senso opposto
   printf("Static %lx Stack %lx Heap %lx\n", (unsigned long)&a, (unsigned long)&x, (unsigned long)p);
   if (x <= 1)
      return 1;
   return x * fattoriale(x-1);
}

int main(int argc, char **argv) {
   if (args != 2) {
      printf("Usage: %s <num>\n", argv[0]);
   }
   printf("Fattorial of %d is %d". atoi(argv[1]), fattoriale(atoi(argv[1])));
}