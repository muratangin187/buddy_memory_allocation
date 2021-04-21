#include <stdlib.h>
#include <stdio.h>
#include "sbmem.h"
#define ASIZE 240
int main()
{
    int i, ret;
    char *p;
    ret = sbmem_open();
    if (ret == -1)
        exit (1);
    p = sbmem_alloc (ASIZE); // allocate space to for characters
    for (i = 0; i < ASIZE; ++i)
        p[i] = 'a'; // init all chars to ‘a’
    printf("%s", p);
//    sbmem_free (p);
    sbmem_close();
    return (0);
}