#include <stdio.h>
#include<stdlib.h>
#include <string.h>

int devuelve_5 () {
    return 5;
}

int main() {
    void* n = devuelve_5();
    //void* i = NULL;
    if (!0) {
        //int sock = *(int *)n;
        printf("hola \n");
    }
    // No puedo hacer strcmp con NULL
    /*if(strcmp("hola", NULL)) {
        printf("strcmp con NULL");
    }*/
    free(NULL);
    return 0;
}