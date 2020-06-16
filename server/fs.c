#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	
	return newInumber;
}

/*Receives a number of buckets indicating the the size of the hashtable and allocs its memory*/
tecnicofs* new_tecnicofs(int numBuckets){
	tecnicofs*fs = malloc(sizeof(tecnicofs));

	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->hashTable = malloc(numBuckets * sizeof(fs->hashTable));

	for (int i = 0; i < numBuckets; i++){
		fs->hashTable[i] = malloc (sizeof(hashBsts));
		fs->hashTable[i]->bstRoot = NULL;
		iniciateMutexs(&(fs->hashTable[i]->mutexBst));	
	}
	
	fs->nextINumber=0;
	return fs;
}

/*Frees the allocated memory for the given number of buckets*/
void free_tecnicofs(tecnicofs* fs, int numBuckets){
	
	for (int i = 0;i < numBuckets; i++){
		free_tree(fs->hashTable[i]->bstRoot);
		destroyThread(&(fs->hashTable[i]->mutexBst));
		free(fs->hashTable[i]);
	}

	free(fs->hashTable);
	free(fs);
}

/*Creates a new file and puts it in the bst*/
void create(tecnicofs* fs, char *name, int inumber, int hash_num){
	
	fs->hashTable[hash_num]->bstRoot = insert(fs->hashTable[hash_num]->bstRoot, name, inumber);
}

/*Deletes a file and removes it from the bst*/
void delete(tecnicofs* fs, char *name, int hash_num){
	
	fs->hashTable[hash_num]->bstRoot = remove_item(fs->hashTable[hash_num]->bstRoot, name);
}

/*Searches a file*/
int lookup(tecnicofs* fs, char *name, int hash_num){
	
	node* searchNode = search(fs->hashTable[hash_num]->bstRoot, name);
	if ( searchNode ) return searchNode->inumber;
	return -1;
}

/*Renames a file if it does not exit one with the current name or one with the new name.*/
int changeName(tecnicofs* fs, char *name, char *newName, int hash_num, int numBuckets){
	int hash_newNum = hash(newName, numBuckets);
	int inumber;
	
	if (hash_num > hash_newNum){
		lockThreads(0,&(fs->hashTable[hash_newNum]->mutexBst));
		lockThreads(0,&(fs->hashTable[hash_num]->mutexBst));

		if ((inumber = lookup(fs, name, hash_num))==-1 || lookup(fs, newName, hash_newNum)!=-1){
			unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
			unlockThreads(&(fs->hashTable[hash_newNum]->mutexBst));
			return -1;
		}

		delete(fs, name, hash_num);
		create(fs, newName, inumber, hash_newNum);

		unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
		unlockThreads(&(fs->hashTable[hash_newNum]->mutexBst));

	}else if(hash_num == hash_newNum){

		lockThreads(0,&(fs->hashTable[hash_num]->mutexBst));

		if ((inumber = lookup(fs, name, hash_num))==-1 || lookup(fs, newName, hash_newNum)!=-1){
			unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
			return -1;
		}
		delete(fs, name, hash_num);
		create(fs, newName, inumber, hash_newNum);
		
		unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
	}else{
		lockThreads(0,&(fs->hashTable[hash_num]->mutexBst));
		lockThreads(0,&(fs->hashTable[hash_newNum]->mutexBst));

		if ((inumber = lookup(fs, name, hash_num))==-1 || lookup(fs, newName, hash_newNum)!=-1){
			unlockThreads(&(fs->hashTable[hash_newNum]->mutexBst));
			unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
			return -1;
		}
		delete(fs, name, hash_num);
		create(fs, newName, inumber, hash_newNum);

		unlockThreads(&(fs->hashTable[hash_newNum]->mutexBst));
		unlockThreads(&(fs->hashTable[hash_num]->mutexBst));
	}
	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs, int hash_num){
	print_tree(fp, fs->hashTable[hash_num]->bstRoot);
}