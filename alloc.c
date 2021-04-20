#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

int main(void)
{
    int x = 128;
    int n = ceil(log(x) / log(2));
    size = n + 1;
    struct linkedList * link[size];
    for(int i = 0; i < size; i++){
        link[i] = (struct linkedList *) malloc (sizeof(struct linkedList));
        linkedList_init(link[i]);
    }

    struct node *qe = (struct node *) malloc (sizeof(struct node));
    qe->next = NULL;
    qe->chunk.start = 0;
    qe->chunk.end = x-1;

    pushToTail( link[size-1], qe );

    display(link, size);
    allocate(link, 8);
    allocate(link, 8);
    display(link, size);

    int y = deleteNode(link[1], 15250);
    printf("y = %d\n", y);
    return 0;
}