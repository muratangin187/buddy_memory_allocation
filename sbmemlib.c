#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <fcntl.h> // For constants O_CREAT, O_RDWR
#include <sys/mman.h> // For shm_open
#include <unistd.h> // For ftruncate
#include <semaphore.h>
#include "sbmem.h"

void* g_ptr;
void* g_userPtr;
int g_segmentSize;
int* g_currentIndex;
int* g_userCount;

sem_t *sem_users;
sem_t *sem_mutex;

int getOrderFreeCount(int order){
    int count = 0;
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 0 && current->order == order){
            count++;
        }
    }
    return count;
}

Chunk* getOrderHead(int order){
    Chunk* result = (Chunk*)g_ptr;
    for(int i=1; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 0 && current->order == order && current->start < result->start){
            result = current;
        }
    }
    return result;
}

int getMaxOrder(int segmentSize){
    return ceil(log(segmentSize) / log(2));
}

int detectNeededMemorySize(int segmentSize){
    return (int)(pow(2, getMaxOrder(segmentSize)-7)*sizeof(Chunk)) + 3*sizeof(int);
}

void deleteChunk(int start){
    int found = 0;
    (*g_currentIndex)--;
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 0 && current->start == start){
            found = 1;
        }
        if(found == 1){
            Chunk* next = ((Chunk*)(g_ptr+(i+1)*sizeof(Chunk)));
            current->start = next->start;
            current->end = next->end;
            current->isAllocated = next->isAllocated;
            current->order= next->order;
            current->pid= next->pid;
            current->usedSize= next->usedSize;
        }
    }
}

Chunk* chunkIsAllocated(int start){
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 1 && current->start == start){
            return current;
        }
    }
    return NULL;
}

Chunk* searchInFree(int start){
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 0 && current->start == start){
            return current;
        }
    }
    return NULL;
}

void displayFree(){
    printf("Free list:\n");
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 0){
            printf("(%d, %d, %d, %d %d)", current->start, current->end, current->order, current->usedSize, current->pid);
        }
    }
    printf("\n");
}

void displayAllocated(){
    printf("\nAllocated list:\n");
    for(int i=0; i < *g_currentIndex; i++){
        Chunk* current = ((Chunk*)(g_ptr+i*sizeof(Chunk)));
        if(current->isAllocated == 1){
            printf("(%d, %d, %d, %d %d)", current->start, current->end, current->order, current->usedSize, current->pid);
        }
    }
    printf("\n");
}

void display(){
    displayAllocated();
    displayFree();
}

int sbmem_init(int segmentSize)
{
    if(segmentSize == 0 || ceil(log2(segmentSize)) != floor(log2(segmentSize))){
        printf("Segment size must be power of two");
        return -1;
    }

    int shared_fd = shm_open(FDNAME, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if(shared_fd == -1){
        printf("Error on shm_open function");
        return -1;
    }
    if(ftruncate(shared_fd, segmentSize+detectNeededMemorySize(segmentSize)) == -1){
        printf("Error on ftruncate function");
        return -1;
    }
    void* ptr = mmap(0, segmentSize+detectNeededMemorySize(segmentSize), PROT_WRITE, MAP_SHARED, shared_fd, 0);
    if(ptr == MAP_FAILED){
        printf("Error on mmap function");
        return -1;
    }
    void* basePtr = ptr;

    *(int *)ptr = segmentSize; // maximum order
    ptr += sizeof(int);
    *(int *)ptr = 1; // current index of freelist
    ptr += sizeof(int);
    *(int *)ptr = 9; // user count
    ptr += sizeof(int);

    Chunk* chunk = (Chunk*)ptr;
    chunk->start = 0;
    chunk->end = segmentSize-1;
    chunk->isAllocated = 0;
    chunk->order = getMaxOrder(segmentSize);
    chunk->pid = -1;
    chunk->usedSize = -1;
    if(munmap(basePtr, segmentSize+ detectNeededMemorySize(segmentSize)) == -1){
        printf("Error on munmap function");
        return -1;
    }

    sem_unlink(SEM_USERS);
    sem_unlink(SEM_MUTEX);

    /* create and initialize the semaphores */
    sem_users = sem_open(SEM_USERS, O_RDWR | O_CREAT, 0660, 1);
    if (sem_users < 0) {
        perror("Can not create semaphore SEM_USERS\n");
        exit (1);
    }
    printf("Semaphore %s created\n", SEM_USERS);

    sem_mutex = sem_open(SEM_MUTEX, O_RDWR | O_CREAT, 0660, 1);
    if (sem_mutex < 0) {
        perror("Can not create semaphore SEM_MUTEX\n");
        exit (1);
    }
    printf("Semaphore %s created\n", SEM_MUTEX);

    return 0;
}

int sbmem_remove()
{
    shm_unlink(FDNAME);
    sem_unlink(SEM_USERS);
    sem_unlink(SEM_MUTEX);
    return (0);
}

int sbmem_open()
{
    sem_users = sem_open(SEM_USERS, O_RDWR);
    sem_wait(sem_users);

    int shm_fd = shm_open(FDNAME, O_RDWR, 0666);
    g_ptr = mmap(0, sizeof(int), PROT_READ, MAP_SHARED, shm_fd, 0);
    g_segmentSize = *(int *)(g_ptr);
    munmap(g_ptr, sizeof(int));

    g_ptr = mmap(0, g_segmentSize+detectNeededMemorySize(g_segmentSize), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    g_userPtr = g_ptr + detectNeededMemorySize(g_segmentSize);
    g_ptr += sizeof(int);
    g_currentIndex = (int *)(g_ptr);
    g_ptr += sizeof(int);
    g_userCount = (int *)(g_ptr);
    if(*g_userCount == 10){
        printf("At most 10 processes will request memory");
        sem_post(sem_users);
        return -1;
    }
    (*g_userCount)++;
    g_ptr += sizeof(int);
//    displayAllocated();
//    displayFree();

    sem_post(sem_users);
    return 0;
}


void *sbmem_alloc (int requested_size)
{
    sem_mutex = sem_open(SEM_MUTEX, O_RDWR);
    sem_wait(sem_mutex);
    //get size from shared memory
    int n = ceil(log(requested_size) / log(2));
    if(getOrderFreeCount(n) > 0){
        Chunk* tmp = getOrderHead(n);
        int parent_start = tmp->start;
        int parent_end = tmp->end;
        deleteChunk(tmp->start);
        printf("Allocated %d to %d.\n", parent_start, parent_end);
        Chunk* newKeyValue = g_ptr+((*g_currentIndex)*sizeof(Chunk));
        (*g_currentIndex)++;
        newKeyValue->start = parent_start;
        newKeyValue->end = parent_end;
        newKeyValue->isAllocated = 1;
        newKeyValue->order = n;
        newKeyValue->pid = getpid();
        newKeyValue->usedSize = requested_size;
//        displayAllocated();
//        displayFree();
        sem_post(sem_mutex);
        return g_userPtr+parent_start;
    }else{
        int i;
        for (i = n + 1; i <= getMaxOrder(g_segmentSize); ++i){
            if(getOrderFreeCount(i) != 0)
                break;
        }
        if(i == getMaxOrder(g_segmentSize)+1){
            printf("Failed to allocate memory\n");
            sem_post(sem_mutex);
            return NULL;
        }else{
            Chunk* tmp = getOrderHead(i);
            int parent_start = tmp->start;
            int parent_end = tmp->end;
            int parent_order = tmp->order;
            deleteChunk(tmp->start);
            i--;
            for(; i >= n; i--){
                Chunk* tmp1 = g_ptr+(*g_currentIndex)*sizeof(Chunk);
                (*g_currentIndex)++;
                Chunk* tmp2 = g_ptr+(*g_currentIndex)*sizeof(Chunk);
                (*g_currentIndex)++;
                //Malloc needed
                tmp1->start = parent_start;
                tmp1->end = parent_start + ((parent_end - parent_start) / 2);
                tmp1->isAllocated = 0;
                tmp1->order = i;
                tmp1->pid = -1;
                tmp1->usedSize = -1;

                tmp2 -> start = parent_start + (parent_end - parent_start)/2 + 1;
                tmp2-> end = parent_end;
                tmp2->isAllocated = 0;
                tmp2->order = i;
                tmp2->pid = -1;
                tmp2->usedSize = -1;

                parent_start = tmp1->start;
                parent_end = tmp1->end;
                parent_order = tmp1->order;
                deleteChunk(getOrderHead(i)->start);
            }

            printf("Allocated %d to %d.\n", parent_start, parent_end);
            Chunk* newKeyValue = g_ptr+((*g_currentIndex)*sizeof(Chunk));
            (*g_currentIndex)++;
            newKeyValue->start = parent_start;
            newKeyValue->end = parent_end;
            newKeyValue->isAllocated = 1;
            newKeyValue->order = parent_order;
            newKeyValue->pid = getpid();
            newKeyValue->usedSize = requested_size;
//            displayAllocated();
//            displayFree();
            sem_post(sem_mutex);
            return g_userPtr+parent_start;
        }
    }
}

void sbmem_free (void *p)
{
    sem_mutex = sem_open(SEM_MUTEX, O_RDWR);
    sem_wait(sem_mutex);
    int offset = p-g_userPtr;
    Chunk* allocated = chunkIsAllocated(offset);
    if(allocated == NULL){
       printf("Invalid free address\n");
        sem_post(sem_mutex);
        return;
    }
    printf("Freed %d and %d.\n", allocated->start, allocated->end);

    while(allocated != NULL) {
        // TODO check user pid for free request
        allocated->isAllocated = 0;
        allocated->pid = -1;
        allocated->usedSize = -1;
        int allocatedSize = allocated->end- allocated->start+1;

        int n = ceil(log(allocatedSize) / log(2));
        int buddyN;
        int buddyA;
        buddyN = allocated->start / allocatedSize;

        if (buddyN % 2 != 0)
            buddyA = allocated->start - pow(2, n);
        else
            buddyA = allocated->start + pow(2, n);

        Chunk *freeBuddy = searchInFree(buddyA);
        if (freeBuddy != NULL) {
            Chunk *newKeyValue = g_ptr + ((*g_currentIndex) * sizeof(Chunk));
            (*g_currentIndex)++;
            newKeyValue->isAllocated = 0;
            newKeyValue->order = allocated->order + 1;
            newKeyValue->pid = -1;
            newKeyValue->usedSize = -1;
            if (buddyN % 2 == 0) {
                newKeyValue->start = allocated->start;
                newKeyValue->end = allocated->start + 2 * (pow(2, n)) - 1;
                printf("Merged %d and %d.\n", allocated->start, buddyA);
            } else {
                newKeyValue->start = buddyA;
                newKeyValue->end = buddyA + 2 * (pow(2, n));
                printf("Merged %d and %d.\n", buddyA, allocated->start);
            }

            deleteChunk(freeBuddy->start);
            deleteChunk(allocated->start);
            allocated = newKeyValue;
        } else {
            allocated = NULL;
        }
    }
    sem_post(sem_mutex);
}

int sbmem_close()
{
    sem_users = sem_open(SEM_USERS, O_RDWR);
    sem_wait(sem_users);
    (*g_userCount)--;
    munmap(g_ptr-3*sizeof(int), g_segmentSize+detectNeededMemorySize(g_segmentSize));
    sem_post(sem_users);
    return (0);
}