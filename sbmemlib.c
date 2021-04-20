#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <fcntl.h> // For constants O_CREAT, O_RDWR
#include <sys/mman.h> // For shm_open
#include <unistd.h> // For ftruncate
#include "sbmem.h"

struct node* getHead(void* basePtr, int offset){
    return basePtr+offset;
}

void linkedList_insert(struct linkedList *q, struct node *newNode){
    newNode ->next = q->head;

    q->head = newNode;
    q->count++;
}

int deleteNode(struct linkedList *q, int startAddress){
    if(q->count == 0)
        return -1;
    struct node * retrievedNode;
    retrievedNode = q->head;
    struct node * prev = NULL;

    if(retrievedNode != NULL && retrievedNode->chunk.start == startAddress){
        q->head = retrievedNode->next;
        //TODO FREE
        return 1;
    }
    while(retrievedNode != NULL && retrievedNode->chunk.start != startAddress){
        prev = retrievedNode;
        retrievedNode = retrievedNode->next;
    }
    if(retrievedNode == NULL){
        return -1;
    }

    prev->next = retrievedNode->next;
    q->count--;
    //TODO: FREE
    return 1;
}

struct node * search(struct linkedList *q, int startAddress){
    if(q->count == 0)
        return NULL;
    struct node * retrievedNode;
    retrievedNode = q->head;

    if(retrievedNode != NULL && retrievedNode->chunk.start == startAddress){
        q->head = retrievedNode->next;
        return retrievedNode;
    }
    while(retrievedNode != NULL && retrievedNode->chunk.start != startAddress){
        retrievedNode = retrievedNode->next;
    }
    if(retrievedNode == NULL){
        return NULL;
    }
    return (retrievedNode);
}

void pushToTail(struct linkedList *q, struct node *newNode){
    if(q->count == 0){
        q->head = newNode;
    }
    struct node*tail = q->head;
    while(tail->next != NULL )
        tail = tail ->next;
    tail->next = newNode;
    newNode->next = NULL;
    q->count++;
}

void display(struct linkedList **q, int size){
    printf("\n");
    for(int i=0; i<size; i++){
        printf("{");
        struct node* tmp;

        tmp = q[i]->head;
        while(tmp != NULL){
            printf("(%d, %d)", tmp -> chunk.start, tmp -> chunk.end);
            tmp = tmp->next;
        }
        printf("}, ");
    }
    printf("\n");
}

void displayNode(struct node *cur)
{
    printf("(%d", cur->chunk.start);
    printf(",%d", cur->chunk.end);
    printf(")");
}

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.

// Define semaphore(s)

// Define your stuctures and variables.

// calculate needed memory and allocate for library usage
int detectNeededMemorySize(int segmentSize){
    return 2*sizeof(int);
}

int getMaxOrder(int segmentSize){
    return ceil(log(segmentSize) / log(2))+1;
}

int sbmem_init(int segmentSize)
{
    printf ("sbmem init called\n"); // remove all printfs when you are submitting to us.
    if(segmentSize % 2 != 0){
        printf("Error, segment size must be a power of two!\n");
        return -1;
    }

    int shared_fd = shm_open(FDNAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shared_fd, segmentSize+detectNeededMemorySize(segmentSize));
    void* ptr = mmap(0, segmentSize+detectNeededMemorySize(segmentSize), PROT_WRITE, MAP_SHARED, shared_fd, 0);
    void* basePtr = ptr;
    *(int *)ptr = segmentSize; // maximum order
    ptr += sizeof(int);
    *(int *)ptr = 1; // current index of freelist
    ptr += sizeof(int);
    struct linkedList* allocatedList = (struct linkedList*)ptr;
    allocatedList->count = 0;
    allocatedList->head = NULL;
    ptr += sizeof(struct linkedList);
    struct linkedList** link = (struct linkedList**)ptr;
    ptr += getMaxOrder(segmentSize)*sizeof(struct linkedList*);
    for(int i = 0; i < getMaxOrder(segmentSize); i++){
        link[i] = (struct linkedList*)ptr;
        link[i]->count = 1;
        link[i]->head = NULL;
        ptr += sizeof(struct linkedList);
    }
    ((struct node*)ptr)->next = NULL;
    ((struct node*)ptr)->chunk.start = 0;
    ((struct node*)ptr)->chunk.end = segmentSize-1;
    linkedList_insert(link[getMaxOrder(segmentSize)-1], (struct node*)ptr);
    munmap(basePtr, segmentSize+ detectNeededMemorySize(segmentSize));

    return 0;
}

int sbmem_remove()
{

    return (0);
}

int sbmem_open()
{

    return (0);
}


void *sbmem_alloc (int requested_size)
{
    //get size from shared memory
    int shm_fd = shm_open(FDNAME, O_CREAT | O_RDWR, 0666);
    void* ptr = mmap(0, sizeof(int), PROT_READ, MAP_SHARED, shm_fd, 0);
    int segmentSize = *(int *)(ptr);
    int maxOrder = getMaxOrder(segmentSize);
    munmap(ptr, sizeof(int));
    ptr = mmap(0, segmentSize+detectNeededMemorySize(segmentSize), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    void* userBasePtr = ptr + detectNeededMemorySize(segmentSize);
    ptr += sizeof(int);
    int* currentIndex = (int *)(ptr);
    ptr += sizeof(int);



    struct linkedList* allocatedList = (struct linkedList*)ptr;
    ptr += sizeof(struct linkedList);
    struct linkedList** freeList = (struct linkedList **) (ptr);
    ptr += getMaxOrder(segmentSize) * (sizeof(struct linkedList*) + sizeof(struct linkedList));

    int n = ceil(log(requested_size) / log(2));
    if(freeList[n] ->count > 0){
        struct node* tmp = freeList[n]->head;
        deleteNode(freeList[n],freeList[n]->head->chunk.start);
        freeList[n]->count--;
        printf("Memory from %d to %d is allocated\n", tmp->chunk.start, tmp->chunk.end);
        struct node* newKeyValue = ptr+(*currentIndex)*sizeof(struct node);
        (*currentIndex)++;
        newKeyValue->next = NULL;
        newKeyValue->chunk.start = tmp->chunk.start;
        newKeyValue->chunk.end = tmp->chunk.end;
        linkedList_insert(allocatedList, newKeyValue);
        return userBasePtr+tmp->chunk.start;
    }

    else{
        int i;
        for (i = n + 1; i < maxOrder; ++i){
            if(freeList[i]->count != 0)
                break;
        }
        if(i == maxOrder){
            printf("Failed to allocate memory\n");
        }

        else{
            struct node* tmp = freeList[i]->head;
            deleteNode(freeList[i],freeList[i]->head->chunk.start);
            freeList[i]->count--;
            i--;
            for(; i >= n; i--){
                struct node* tmp1 = ptr+(*currentIndex)*sizeof(struct node);
                (*currentIndex)++;
                struct node* tmp2 = ptr+(*currentIndex)*sizeof(struct node);
                (*currentIndex)++;
                //Malloc needed
                tmp1->next = NULL;
                tmp1->chunk.start = tmp->chunk.start;
                tmp1->chunk.end = tmp -> chunk.start + ((tmp->chunk.end - tmp->chunk.start) / 2);
                tmp2->next = NULL;
                tmp2 -> chunk.start = tmp->chunk.start + (tmp->chunk.end - tmp->chunk.start)/2 + 1;
                tmp2-> chunk.end = tmp->chunk.end;

                pushToTail(freeList[i], tmp1);
                pushToTail(freeList[i], tmp2);
                tmp = tmp1;
                deleteNode(freeList[i],freeList[i]->head->chunk.start);
                freeList[i]->count--;
            }

            printf("Memory from %d to %d is allocated\n", tmp->chunk.start, tmp->chunk.end);
            struct node* newKeyValue = ptr+(*currentIndex)*sizeof(struct node);
            (*currentIndex)++;
            newKeyValue->next = NULL;
            newKeyValue->chunk.start = tmp->chunk.start;
            newKeyValue->chunk.end = tmp->chunk.end;
            linkedList_insert(allocatedList, newKeyValue);
            return userBasePtr+tmp->chunk.start;
        }

    }
    struct node* tmp;
    tmp = allocatedList->head;
    printf("Allocated chunks: ");
    while(tmp != NULL){
        printf("(%d, %d)", tmp -> chunk.start, tmp -> chunk.end);
        tmp = tmp->next;
    }
}

//
//void sbmem_free (void *p)
//{
//    // if(mp.find(id) == mp.end())
//    // {
//    //   cout << "Sorry, invalid free request\n";
//    //   return;
//    // }
//
//    // int n = ceil(log(mp[id]) / log(2));
//    int n = 10; //Must be above!!!
//    int i;
//    int buddyN;
//    int buddyA;
//    //Malloc needed
//    struct node* tmp1 = (struct node *) malloc (sizeof(struct node));
//    tmp1->next = NULL;
//    tmp1->chunk.start = id;
//    tmp1->chunk.end = id + pow(2,n)-1;
//
//    pushToTail(link[n], tmp1);
//    printf("Memory block from %d to %d is freed\n", tmp1->chunk.start, tmp1->chunk.end);
//
//    // buddyN = id / mp[id];
//    buddyN = id; //Must be changed with upper value
//
//    if(buddyN % 2 != 0)
//        buddyA = id - pow(2,n);
//    else
//        buddyA = id + pow(2,n);
//
//    struct node* tmp = search(link[n], buddyA);
//    if(tmp != NULL){
//        struct node* tmp2 = (struct node *) malloc (sizeof(struct node));
//        if(buddyN % 2 == 0){
//            tmp2->next = NULL;
//            tmp2->chunk.start = id;
//            tmp2->chunk.end = id+ 2*(pow(2,n)-1);
//            pushToTail(link[n], tmp2);
//
//        }
//
//        else {
//            tmp2->next = NULL;
//            tmp2->chunk.start = buddyA;
//            tmp2->chunk.end = buddyA + 2*(pow(2,n));
//            pushToTail(link[n], tmp2);
//        }
//
//        printf("Coalescing of blocks starting at %d and %d was done\n", tmp2->chunk.start, tmp2->chunk.end);
//        struct node* tmp3 = link[n]->head;
//        deleteNode(link[n],tmp->chunk.start);
//        deleteNode(link[n] ,getTailStartAddress(link[n]));
//        // (0,15), (16,31), (32,47), (64,79),
//    }
//
//    // mp.erase(id);
//}
//
int sbmem_close()
{

    return (0);
}