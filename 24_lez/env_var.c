#include <stdio.h>

int main(int argc, int *argv[], int *env[]) {
    // print arg var
    for (int i=0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    // print env var
    for (int i=0; env[i]; i++) {
        printf("env[%d]: %s\n", i, env[i]);
    }
    /*
    argv...
    env..
    SHELL = /bin/bash
    PWD = /mnt/2AF2C201D2C1D0ED/Documenti/uni/Reti_di_Calcolatori/Esercizi/24_lez
    USER = kali
    PATH = ...

    */
    return 0;
}