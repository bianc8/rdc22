#include<stdio.h>

int main() {
   char v[3];
   printf("Sono qui");
   // non stampa nulla xk usiamo stream per stampare a schermo,
   // la printf ha mandato la stringa, che è bufferizzata
   // e non servita xk rileva segmentation fault
   printf("Sono qui\n");
   // con \n faccio il flush del buffer, quindi sarà stampato a schermo "Sono quiSono qui"
   // resta evidente che non ci sia sync con lo stream
   v[-8700000999] = 'c';   // segmentation fault core dumped
   printf("Sono qua");
   return 0;
}