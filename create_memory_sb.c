#include <stdio.h>
#include <unistd.h>

#include "sbmem.h"

int main()
{

    sbmem_init(262144);
//    int *p = sbmem_alloc(10);
//    if(*p == 5){
//        printf("Already allocated");
//    }else{
//        *p = 5;
//    }
//    sbmem_alloc(10);
//    sbmem_alloc(100);
//    sbmem_alloc(100);
    printf ("memory segment is created and initialized \n");

    return (0);
}