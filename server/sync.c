#include "sync.h"
#include <stdio.h>
#include <stdlib.h>

/*Iniciates the required mutex*/
void iniciateMutexs(void * arg){

    if (pthread_rwlock_init((pthread_rwlock_t * )arg, NULL)){
        fprintf(stderr,"Error: thread not iniciated.\n");
        exit(EXIT_FAILURE);
    }
}

/*Locks the correct thread, if flag==0 locks wrlock, else locks rdlock.*/
void lockThreads(int flag, void* arg){

    if (flag == 0){ 
        if (pthread_rwlock_wrlock((pthread_rwlock_t * )arg)){
            fprintf(stderr,"Error: Unable to lock.\n");
            exit(EXIT_FAILURE);
        }  
    }
    else{
        if (pthread_rwlock_rdlock((pthread_rwlock_t * )arg)){
            fprintf(stderr,"Error: Unable to lock.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*Unlocks the corresponding lock*/
void unlockThreads(void* arg){

    if(pthread_rwlock_unlock((pthread_rwlock_t * )arg)){
        fprintf(stderr, "Error: Enable to unlock.\n");
        exit(EXIT_FAILURE);
    }
}

/*Deletes the correspondig thread*/
void destroyThread(void* arg){
    
    if(pthread_rwlock_destroy((pthread_rwlock_t * )arg)){
        fprintf(stderr, "Error: Enable to destroy.\n");
        exit(EXIT_FAILURE);
    }
}