/*Filipa Malta 93708
  Martim Pimentel 93738*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "fs.h"
#include "lib/hash.h"
#include "sync.h"
#include "lib/inodes.h"
#include "../client/tecnicofs-api-constants.h"
#include "ficheiros.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define END_COMMAND "v v"
#define NUM_THREADS 15
#define MAX_CLIENTS 5

int numberBuckets = 0;
int serv_sockfd;
int runFlag = 1;

struct timeval start, end;
tecnicofs* fs;

static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}

/*Servidor*/
int createSocket(char* address){
    int serv_sockfd;
    socklen_t dim_serv;
    struct sockaddr_un end_serv;

/*cria socket stream*/
    if((serv_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

/*elimina preventivamente o ficheiro ou socket prexistente*/
    unlink(address);

/*preenche a estrutura com o nome do socket e dimensao ocupada apos limpeza preventiva*/
    bzero((char*)&end_serv, sizeof(end_serv));
    end_serv.sun_family = AF_UNIX;
    strcpy(end_serv.sun_path, address);
    dim_serv = strlen(end_serv.sun_path) + sizeof(end_serv.sun_family);

/*atribuicao do nome ao socket*/
    if(bind(serv_sockfd, (struct sockaddr *) &end_serv, dim_serv) < 0){
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }

/*disponibilidade para esperar pedidos de ligacao*/
    if (listen(serv_sockfd, MAX_CLIENTS)){
        fprintf(stderr, "Unable to listen.\n");
        exit(EXIT_FAILURE);
    }

    return serv_sockfd;
}

/*Verifica se um ficheiro existe*/
int existsFile(char *name, int hash_val){
    lockThreads(1, &(fs->hashTable[hash_val]->mutexBst));
    int searchResult = lookup(fs, name, hash_val);
    unlockThreads(&(fs->hashTable[hash_val]->mutexBst));

    return searchResult;
}

/*Faz os comandos basicos(create, delete, rename, open, close, read, write) com as devidas verificacoes.
Acaba quando ve o END_COMMAND.*/
void applyCommands(Fich_aberto *tabela_logs, char *command, int sockfd, unsigned int my_uid){
    char token;
    char field1[MAX_INPUT_SIZE];
    char field2[MAX_INPUT_SIZE];
    int code=0;
        
    int numTokens = sscanf(command, "%c %s %[^\t\n]", &token, field1, field2);
    
    /* perform minimal validation */
    switch (token) {
        case 'c':
        case 'r':
        case 'o':
        case 'l':
        case 'w':
            if (numTokens != 3){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                return;
            }
            break;
        case 'd':
        case 'x':
            if (numTokens != 2){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                return;
            }
            break;
        default: { /* error */
            code=TECNICOFS_ERROR_OTHER;
            write(sockfd,&code,sizeof(int));
            return;
        }
    }

    if (command == NULL){
        code=TECNICOFS_ERROR_OTHER;
        write(sockfd,&code,sizeof(int));
        return;
    }

    int hash_val = hash(field1, numberBuckets);
    int inumber, fd;
    permission ownerPerm, othersPerm;
    
    switch (token) {
        case 'c':
            ;
            int permission = atoi(field2);
            if (existsFile(field1, hash_val) != -1){
                code=TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                write(sockfd,&code,sizeof(int));
                break;
            }
            
            inumber = inode_create(my_uid, (int)permission/10, (int)permission%10);
            if (inumber == -1){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                return;
            }

            lockThreads(0,&(fs->hashTable[hash_val]->mutexBst));
            create(fs, field1, inumber, hash_val);
            unlockThreads(&(fs->hashTable[hash_val]->mutexBst));

            write(sockfd,&code,sizeof(int));
            break;

        /*Delete/Rename seguem a opcao 1(apenas pode apagar/renomear 
        se nenhum ficheiro do mesmo nome estiver aberto.*/
        case 'd':
            ;
            inumber = existsFile(field1, hash_val);
            uid_t owner_delete;

            if (inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_FOUND;
                write(sockfd,&code,sizeof(int));
                break;
            }
            if (inode_get(inumber, &owner_delete, NULL, NULL, NULL, 0)){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }
            
            if (my_uid != owner_delete){
                code=TECNICOFS_ERROR_PERMISSION_DENIED;
                write(sockfd,&code,sizeof(int));
                break;
            }      

            if (inode_isFileOpen(inumber)){
                code=TECNICOFS_ERROR_FILE_IS_OPEN;
                write(sockfd,&code,sizeof(int));
                break;
            }

            lockThreads(0, &(fs->hashTable[hash_val]->mutexBst));
            delete(fs, field1, hash_val);
            unlockThreads(&(fs->hashTable[hash_val]->mutexBst));

            if (inode_delete(inumber)){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }
            write(sockfd,&code,sizeof(int));
            break;

        case 'r':
            ;
            uid_t owner_rename;
            inumber = existsFile(field1, hash_val);

            if (inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_FOUND;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (inode_get(inumber, &owner_rename, NULL, NULL, NULL, 0)){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (my_uid != owner_rename){
                code=TECNICOFS_ERROR_PERMISSION_DENIED;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (inode_isFileOpen(inumber)){
                code= TECNICOFS_ERROR_FILE_IS_OPEN;
                write(sockfd,&code,sizeof(int));
                break;
            }
            
            if (changeName(fs, field1, field2, hash_val, numberBuckets)){
                code=TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                write(sockfd,&code,sizeof(int));
                break;
            }
            
            write(sockfd,&code,sizeof(int));
            break;

        case 'o':
            ;
            inumber = existsFile(field1, hash_val);
            uid_t owner_open;
            int returnValue;

            if (inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_FOUND;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (inode_get(inumber, &owner_open, &ownerPerm, &othersPerm, NULL, 0)){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (my_uid == owner_open){
                if (ownerPerm != 3 && ownerPerm != atoi(field2)){
                    code=TECNICOFS_ERROR_PERMISSION_DENIED;
                    write(sockfd,&code,sizeof(int));
                    break;
                }

            }else{
                if (othersPerm != 3 && othersPerm != atoi(field2)){
                    code=TECNICOFS_ERROR_PERMISSION_DENIED;
                    write(sockfd,&code,sizeof(int));
                    break;
                }
            }

            int open_flag = inode_OpenFile(inumber);
            if (open_flag){
                write(sockfd,&open_flag,sizeof(int));
                break;
            }

            returnValue = insertOpenFile(tabela_logs, inumber, atoi(field2));
            write(sockfd,&returnValue,sizeof(int));
            break;

        case 'x':
            ;
            fd = atoi(field1);

            if (tabela_logs[fd].inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_OPEN;
                write(sockfd,&code,sizeof(int));
                break;
            }

            int close_flag = inode_closeFile(tabela_logs[fd].inumber);
            if (close_flag){
                write(sockfd,&close_flag,sizeof(int));
                break;
            }

            returnValue = closeOpenedFile(tabela_logs, tabela_logs[fd].inumber);
            write(sockfd,&returnValue,sizeof(int));
            break;

        case 'l':
            ;
            fd = atoi(field1);
            inumber = tabela_logs[fd].inumber;
            char fileContents[MAX_INPUT_SIZE]={0};
            
            if (inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_OPEN;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if(tabela_logs[fd].modoLeitura == 1){
                code=TECNICOFS_ERROR_INVALID_MODE;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if(inode_get(inumber, NULL, NULL, NULL, fileContents, atoi(field2)) == -1){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }
            
            write(sockfd,&code,sizeof(int));
            dprintf(sockfd, "%s%s", fileContents, "\0");
            break;

        case 'w':
            ;
            fd = atoi(field1);
            inumber = tabela_logs[fd].inumber;

            /*inumber e' o que esta contido na tablela de fich abertos*/
            if (inumber == -1){
                code=TECNICOFS_ERROR_FILE_NOT_OPEN;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (tabela_logs[fd].modoLeitura == 2){
                code=TECNICOFS_ERROR_INVALID_MODE;
                write(sockfd,&code,sizeof(int));
                break;
            }

            if (inode_set(inumber, field2, strlen(field2))){
                code=TECNICOFS_ERROR_OTHER;
                write(sockfd,&code,sizeof(int));
                break;
            }

            write(sockfd,&code,sizeof(int));
            break;

        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*Processa o input e inicializa a tabela de ficheiros abertos*/
void *processInput(void* infoArray){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    unsigned int *info = ((unsigned int*) infoArray);
    unsigned int sockfd = info[0];
    unsigned int my_uid = info[1];
    int code=0;
    Fich_aberto *tabela_logs;

    tabela_logs = ficheiro_init();
    if(!tabela_logs){
        code=TECNICOFS_ERROR_OTHER;
        write(sockfd,&code,sizeof(int));
        return NULL;
    }

    while (1){
        char buffer[MAX_INPUT_SIZE] = {0};
        char *command = malloc((MAX_INPUT_SIZE + 1)*sizeof(char));
        if (!command) {
            code=TECNICOFS_ERROR_OTHER;
            write(sockfd,&code,sizeof(int));
            return NULL;
        }
        
        int n = read(sockfd, buffer, MAX_INPUT_SIZE);

        if (n < 0){
            code=TECNICOFS_ERROR_OTHER;
            write(sockfd,&code,sizeof(int));
            return NULL;
        }

        strcpy(command, buffer);

        if (strcmp(command,END_COMMAND) == 0){
            free(command);
            ficheiro_destroy(tabela_logs);
            return NULL;
        }
        
        applyCommands(tabela_logs,command, sockfd, my_uid);
        
        free(command);
    }
    return NULL;
}

/*Funcao de controlo do sinal*/
void catchCTRLC(int sig){
    runFlag = 0;
    if (close(serv_sockfd)){
        perror("Unable to close Server SocketFd.\n");
        exit(EXIT_FAILURE);
    }
}

/*Aceita novos clientes, cria uma canal de comunicacao e processa-os*/
void processData(int serv_sockfd){
    int newcli_sockfd,code=0;
    socklen_t dim_cli,len;
    struct sockaddr_un end_cli;
    pthread_t clients[NUM_THREADS];
    struct ucred ucred;
    int numThreads = 0;
    unsigned int infoArray[2];
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);

    act.sa_handler = catchCTRLC;
    /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("Sigaction Failed.\n");
        exit(EXIT_FAILURE);
    }
    
    for(;;){
        dim_cli = sizeof(end_cli);
        
        /*sincroniza com o cliente ao aceitar pedido de ligacao: criacao de um canal privado*/
        newcli_sockfd = accept(serv_sockfd, (struct sockaddr *) &end_cli, &dim_cli);
        
        if (runFlag == 0){
            break;
        }
        if(newcli_sockfd < 0){
            fprintf(stderr, "Unable to accept client\n");
            exit(EXIT_FAILURE);
        }

        len = sizeof(struct ucred);
        /*ve qual o uid do cliente */
        if (getsockopt(newcli_sockfd, SOL_SOCKET, SO_PEERCRED, &ucred, &len)){
            code=TECNICOFS_ERROR_OTHER;
            write(newcli_sockfd,&code,sizeof(int));
            continue;
        }

        infoArray[0] = newcli_sockfd;
        infoArray[1] = ucred.uid;
        if(pthread_create(&clients[numThreads], 0, processInput,(void*)&infoArray)){
            code=TECNICOFS_ERROR_OTHER;
            write(newcli_sockfd,&code,sizeof(int));
        }

        numThreads++;
    }

    for (int i = 0; i < numThreads; i++){
        if (pthread_join(clients[i], NULL)){
            fprintf(stderr,"Error: thread not finished.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*Prints the total time of execution*/
void printExecutionTime(){
    printf("TecnicoFS completed in %0.4f seconds.\n", ((end.tv_sec - start.tv_sec)+ 0.000001 * (end.tv_usec - start.tv_usec)));
}

/*Puts the tree inside the given outputFile*/
void exportTree(char* outputFile){

    FILE *outTree; 
    if (!(outTree = fopen(outputFile, "w"))){
        fprintf(stderr,"Error: File not found.\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < numberBuckets; i++){
        print_tecnicofs_tree(outTree, fs, i);
    }

    if (fclose(outTree)){
        fprintf(stderr, "Error: File failed to close.\n");
        exit(EXIT_FAILURE);
    }
}

/*Verifica o numero de buckets*/
void checkNumBuckets(int num){
    if (num <= 0){
        fprintf(stderr,"Error: Number of buckets is equal or less than 0.\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {

    parseArgs(argc, argv);

    numberBuckets = atoi(argv[3]);
    checkNumBuckets(numberBuckets);

    fs = new_tecnicofs(numberBuckets);
    
    inode_table_init();
    
    serv_sockfd = createSocket(argv[1]);

    if (gettimeofday(&start, NULL)){
        fprintf(stderr, "Error: Failed to get inicial time.\n");
        exit(EXIT_FAILURE);
    }
    
    processData(serv_sockfd);

    if (gettimeofday(&end, NULL)){
        fprintf(stderr, "Error: Failed to get final time.\n");
        exit(EXIT_FAILURE);
    }

    exportTree(argv[2]);
    
    printExecutionTime();

    free_tecnicofs(fs, numberBuckets);
    inode_table_destroy();

    exit(EXIT_SUCCESS);
}
