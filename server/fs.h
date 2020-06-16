#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "lib/hash.h"
#include "sync.h"
#include <pthread.h>

typedef struct hashBsts {
    node* bstRoot;

    pthread_rwlock_t mutexBst;
} hashBsts;

typedef struct tecnicofs {
    hashBsts** hashTable;
    int nextINumber;
} tecnicofs;


int obtainNewInumber(tecnicofs* fs);
void iniciateMutexs(void * arg);
tecnicofs* new_tecnicofs(int numBuckets);
void free_tecnicofs(tecnicofs* fs, int numBuckets);
void create(tecnicofs* fs, char *name, int inumber,int hash_num);
void delete(tecnicofs* fs, char *name,int hash_num);
int lookup(tecnicofs* fs, char *name,int hash_num);
int changeName(tecnicofs* fs, char *name, char *newName, int hash_num, int numBuckets);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs, int hash_num);

#endif /* FS_H */