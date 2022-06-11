#include <stdio.h>
#include <unistd.h>

char alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "0123456789+/";

/*
H        e        l        l        o               ASCII
01001000 01100101 01101100 01101100 01101111        BINARY
010010  000110 010101 101100 011011 000110 111100   base64 binary
                                    32 16 8 4 2 1
010010  000110 010101 101100 011011 000110 1111=    base64 binary
18      6      21     44     27     6      60   
S       G      V      s      b      G      8= 
*/


int main()
{
    int r;
    unsigned char o[3], s[4];

    while((r = read(0, o, 3)) > 0) {
        s[0] = alphabet[o[0] >> 2];
        s[1] = alphabet[((o[0] & 0x3) << 4) | (o[1] >> 4)];
        s[2] = alphabet[((o[1] & 0xF) << 2) | (o[2] >> 6)];
        s[3] = alphabet[o[2] & 0x3F];

        if (r == 1) {
            s[1] = alphabet[(o[0] & 0x3) << 4];
            s[2] = s[3] = '=';
        }

        if (r == 2) {
            s[2] = alphabet[(o[1] & 0xF) << 2];
            s[3] = '=';
        }

        write(1, s, 4);
    }
}