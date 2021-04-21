#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc50-cpp"
#pragma clang diagnostic ignored "-Wint-conversion"
#pragma ide diagnostic ignored "cert-msc51-cpp"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "sbmem.h"
int main()
{
    int ret;
    ret = sbmem_open();
    if (ret == -1)
        exit (1);
    void **ptrs = sbmem_alloc(sizeof(void *));

    srand(NULL);
    for (int i = 0; i <= 128; i++) {
        ptrs[i] = sbmem_alloc((rand()%1024)+1);
    }

    display();
    for (int i = 0; i <= 128; i++) {
        sbmem_free(ptrs[i]);
    }
    sbmem_free(ptrs);
//    sbmem_free (p);
    sbmem_close();
    return (0);
}


#pragma clang diagnostic pop