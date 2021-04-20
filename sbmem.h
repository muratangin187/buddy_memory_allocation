#ifndef PROJECT3_SBMEM_H
#define PROJECT3_SBMEM_H
#define FDNAME "/project3"

struct chunk {
    int start;
    int end;
};

struct node {
    struct node *next;
    struct chunk chunk;
};

struct linkedList {
    struct node* head;
    int count;
};

int sbmem_init(int segmentsize);
int sbmem_remove();

int sbmem_open();
void *sbmem_alloc (int size);
void sbmem_free(void *p);
int sbmem_close();


#endif //PROJECT3_SBMEM_H
