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

    for (int i = 0; i <= 10; i++) {
        ptrs[i] = sbmem_alloc((int)pow(2,i+7));
    }

    display();
    for (int i = 0; i <= 10; i++) {
        sbmem_free(ptrs[i]);
    }
    sbmem_free(ptrs);
//    sbmem_free (p);
    sbmem_close();
    return (0);
}

