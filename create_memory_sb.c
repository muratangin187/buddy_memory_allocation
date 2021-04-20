#include <stdio.h>
#include <unistd.h>

#include "sbmem.h"

int main()
{

    sbmem_init(262144);
//    sbmem_open();
//    int *p = sbmem_alloc(128);
//    int *t = sbmem_alloc(16);
//    int *s = sbmem_alloc(16);
//    if(*p == 5){
//        printf("Already allocated");
//    }else{
//        *p = 5;
//    }
//    sbmem_alloc(10);
//    sbmem_alloc(100);
//    sbmem_alloc(100);
//    printf ("memory segment is created and initialized \n");

    return (0);
}