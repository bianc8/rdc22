#include <stdio.h>
#include <stdint.h>

/*
gcc file.c -o file -g // Per compilare con debug si ottiene la symbol table, dove si vede che variable è associata ai vari indirizzi
gdb file       // debugger
run [argv]   // esegue il programma come se fosse da linea di comando

come interrompere debugger e vedere stato variabili? Si aggiunge un breaking point
break nome_funzione  // crea breakpoint 1
break linea // crea breakpoint 2
delete 1  // cancello breakpoint con alias 1
run       // esegue il programma e ferma esecuzione alla riga break

Ora si può eseguire questi comandi:
n         // next instructions 
print x   // stampa variabili
display x // continua a stampare variabile x ad ogni esecuzione
c         // continue a eseguire fino al breakpoint successivo
where     // fa vedere stack chiamate? (non sono sicuro)
set x = -1  // imposta valore di una variabile in esecuzione  
quit      // uscita da debugger
*/
struct complesso {
   float re;
   float im;
};

struct xxx {
   char a;
   int b;
};

struct yyy {
   int b;
   char a;
};

struct zzz {
   int a;
   char b;
   long double c;
   char d;
   char e;
};

int main() {
   printf("float - float\n");
   struct complesso c; // c.re init 0x4000
   c.re = 1.0;
   c.im = -2.5;
   // strutture si ha la certezza che i dati che vengono posti in strutture, vengo giustapposti in memoria senza spazi
   // &c == &c.re --> 0x4000
   //       &c.im --> 0x4004
   printf("%p\n", &c.re); // 0x4000
   printf("%p\n", &c.im); // 0x4004

   //pt 2
   printf("\nchar int (padding)\n");
   struct xxx x;
   x.a = 'a';  // &x == &x.a --> 0x5000 char lungo 1 byte + 3 byte di padding (REGOLA DI AGGIUSTAMENTO)
   x.b = 1;    //       &x.b --> 0x5004
   printf("%p\n", &x.a); // 0x5008
   printf("%p\n", &x.b); // 0x500c

   printf("\nint - char\n");
   struct yyy y;
   y.b = 1;    // &y == &y.b --> 0x5000
   y.a = 'z';   //      &y.a --> 0x5004 char lungo 1 byte
   printf("%p\n", &y.b); // 0x5000 
   printf("%p\n", &y.a); // 0x5004

   printf("\nint - char - long double - char (padding)\n");
   struct zzz z;
   z.a = 10;	// &z == &z.a == 0x6000
   z.b = 'a';	//       &z.b == 0x6004
   z.c = 100.10;//       &z.c == 0x6000
   z.d = 'b';	//       &z.d == 0x6000
   z.e = 'c';	//       &z.e == 0x6001
   printf("a %p\n", &z.a);
   printf("b %p\n", &z.b);
   printf("c %p\n", &z.c);
   printf("d %p\n", &z.d);
   printf("e %p\n", &z.e);
   printf("struct %ld - char %ld - int %ld - long %ld - long double %ld\n", sizeof(z), sizeof(char), sizeof(int), sizeof(long), sizeof(long double));

   // pt 3
   int8_t carattere = 'v';  // 0x76 ascii
   int16_t corto = 10;	    // 0xa hex
   int32_t intero = 100;    // 0x64 hex
   int64_t lungo = 10.0;    // 0xa hex

   printf("\nint8 as char: %x\n", *(char*)&carattere);
   printf("int16 as corto: %x\n", *(char*)&corto);
   printf("int32 as int: %x\n", *(int*)&intero);
   printf("int64 as lungo: %lx\n", *(long*)&lungo);

   return 0;
}
