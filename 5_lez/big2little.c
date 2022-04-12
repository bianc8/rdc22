#include<stdio.h>
#include <stdint.h>

void bigLittle(void* point_v, size_t size) {
   char* p = point_v;
   size_t start, end;
   char tmp;
   
   for (start = 0, end = size - 1; end > start; start++, end--) {
      tmp = *(p + start);
      *(p + start) = *(p + end);
      *(p + end) = tmp;
   }
}

int main() {
   uint32_t a = 1;      // little endian  01 00 00 00
                        // big endian     00 00 00 01
   int* p = &a;
   size_t size = sizeof(int);
   bigLittle(p, size);
   printf("%d\n", *p);  // 16777216 = 2^24

   return 0;
}