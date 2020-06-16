# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-g -Wall -std=gnu99 -I../
LDFLAGS=-lm -pthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs


tecnicofs: lib/bst.o fs.rwlock.o lib/hash.o main.rwlock.o lib/inodes.o ficheiros.rwlock.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs lib/bst.o fs.rwlock.o lib/hash.o lib/inodes.o main.rwlock.o sync.rwlock.o ficheiros.rwlock.o 

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

fs.rwlock.o: fs.c fs.h lib/bst.h lib/hash.h sync.rwlock.o
	$(CC) $(CFLAGS) -o fs.rwlock.o -c fs.c

sync.rwlock.o: sync.h sync.c
	$(CC) $(CFLAGS) -o sync.rwlock.o -c sync.c

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

main.rwlock.o: main.c fs.h lib/bst.h lib/hash.h ficheiros.h ../client/tecnicofs-api-constants.h sync.rwlock.o 
	$(CC) $(CFLAGS) -o main.rwlock.o -c main.c

lib/inodes.o: lib/inodes.c lib/inodes.h #../../client/tecnicofs-api-constants.h 
	$(CC) $(CFLAGS) -o lib/inodes.o -c lib/inodes.c

ficheiros.rwlock.o: ficheiros.c ficheiros.h ../client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ficheiros.rwlock.o -c ficheiros.c

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs
