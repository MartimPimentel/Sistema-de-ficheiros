#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

void iniciateMutexs(void * arg);
void lockThreads(int flag, void* arg);
void unlockThreads(void* arg);
void destroyThread(void* arg);

#endif