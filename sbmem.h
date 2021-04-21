#ifndef PROJECT3_SBMEM_H
#define PROJECT3_SBMEM_H
#define FDNAME "/project3"
#define SEM_USERS "/sem_users"
#define SEM_MUTEX "/sem_mutex"

typedef struct Chunk {
    int start;
    int end;
    int order;
    int isAllocated; // 0 or 1
    int pid;
    int usedSize;
} Chunk;

int sbmem_init(int segmentsize);
int sbmem_remove();

int sbmem_open();
void *sbmem_alloc (int size);
void sbmem_free(void *p);
int sbmem_close();
void display();


#endif //PROJECT3_SBMEM_H
