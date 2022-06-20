#include <stdio.h>

int main() {
   int a = 1;
   char c = *(char*)&a;
   if(c == 1) {
      printf("Little endian\n");
   } else {
      printf("Big endian\n");
   }
   
   return 0;
}
