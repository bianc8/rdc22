#include <stdio.h>
#include <string.h>

void raddoppia(int* x) {
   *x = *x + *x;
}

int main() {
   // pt 2 lezione
   int a, b, c;
   a = 1;
   b = 2;
   c = 3;
   int* p;
   p = &a;
   // *(p + (1 * sizeof(int)))
   printf("*(p+1): %d\n", *(p+1)); // print 2 (b)
   printf("p[2]: %d\n", p[2]);   // print 3 (c)

   // pt 3 lezione
   char s[10];
   s[0] = 'C';
   s[1] = 'I';
   s[2] = 'A';
   s[3] = 'O';
   printf("char[] without null terminator: %s\n", s); // "CIAO"
   printf("strlen %lx vs sizeof %ld\n",strlen(s), sizeof(s)); // 4 vs 10
   
   s[4] = 0;
   printf("char[] with null terminator: %s\n", s); // "CIAO"
   printf("strlen %lx vs sizeof %ld\n",strlen(s), sizeof(s)); // 4 vs 10

   int doppio = 2;
   raddoppia(&doppio);
   printf("Doppio: %d\n", doppio);

   return 0;
}