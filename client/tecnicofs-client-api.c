#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "tecnicofs-client-api.h"
#define END_COMMAND "v v"
#define MAX_STRING 101

int sockfd = -1;

/* Estabelece uma sessao com o servidor TecnicoFS localizado no endereço passado como argumento. 
Devolve erro se o cliente já tiver uma sessao ativa com este servidor.*/
int tfsMount(char * address){
	struct sockaddr_un serv_addr;
	socklen_t servlen;

	if((sockfd = socket (AF_UNIX, SOCK_STREAM,0)) < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	/* Primeiro uma limpeza preventiva */
	/*memset*/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	/* Dados para o socket stream: tipo + nome que identifica o servidor */
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, address);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

	if(connect(sockfd,(struct sockaddr *) &serv_addr, servlen)){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return 0;
}

/* Termina uma sessao ativa. 
Devolve erro caso não exista sessao ativa.*/
int tfsUnmount(){
	int len = strlen(END_COMMAND);
	
	if (dprintf(sockfd, "%s%s", "v v", "\0") != strlen(END_COMMAND)){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	if (close(sockfd)){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}
	return 0;
}

/* Cria um novo ficheiro com as permissoes indicadas, cujo dono e' o UID efetivo do processo cliente.
Devolve erro caso já exista um ficheiro com o mesmo nome. */
int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
	int code;
	int size= strlen(filename)+5;

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "c %s %d%d%s", filename, ownerPermissions, othersPermissions, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));
	
	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}

/* Apaga o ficheiro indicado em argumento.
Devolve erro caso o UID efetivo do processo cliente nao seja o mesmo do dono do ficheiro,
caso o ficheiro nao exista ou esteja aberto. */
int tfsDelete(char *filename){
	int code;
	int size= strlen(filename)+2;
	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "d %s%s", filename, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));

	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}

/* Renomeia o ficheiro com o nome indicado no 1 argumento para o novo nome no 2 argumento.
Devolve erro caso nao exista um ficheiro com o nome antigo,o novo nome ja esteja atribuido a um ficheiro,
o UID efetivo do processo cliente nao seja o mesmo do dono do ficheiro ou caso o ficheiro esteja aberto. */
int tfsRename(char *filenameOld, char *filenameNew){
	int code;
	int size=strlen(filenameOld)+ strlen(filenameNew)+3;

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "r %s %s%s", filenameOld, filenameNew, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));

	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}

/* Abre o ficheiro passado por argumento no modo indicado.
Devolve erro caso o ficheiro nao exista, o UID efetivo do processo cliente nao tenha permissao para aceder ao ficheiro 
ou o processo cliente ja tenha esgotado o número máximo de ficheiros que pode abrir.
Retorna o descritor do ficheiro aberto em caso de sucesso. */
int tfsOpen(char *filename, permission mode){
	int code;
	int size= strlen(filename)+4;

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "o %s %d%s", filename, mode, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));
	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}

/* Fecha o ficheiro previamente aberto com o descritor de ficheiro passado por argumento.
Devolve erro caso o descritor não se refira a nenhum ficheiro aberto. */
int tfsClose(int fd){
	int code;
	int size=3;

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "x %d%s", fd, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));

	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}

/* Copia o conteudo do ficheiro aberto ​fd ​para ​buffer.
Devolve erro caso o ficheiro nao esteja aberto ou nao tenha sido aberto em modo de leitura ou leitura/escrita.
Retorna o numero de caracteres lidos em caso de sucesso. */
int tfsRead(int fd, char *buffer, int len){
	int code;
	int size=5;

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (dprintf(sockfd, "l %d %d%s", fd, len -1, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));

	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	if (code < 0){
		return code;
	}else{
		
		char recvline[101] = {0};
		int len = read(sockfd, recvline, 100);
		if (len < 0){
			return TECNICOFS_ERROR_CONNECTION_ERROR;
		}

		recvline[len]='\0';
		strcpy(buffer,recvline);
		return len;
	}
}

/* Substitui o conteudo do ficheiro aberto por uma copia da ​string ​contida no ​buffer passado como argumento.
Devolve erro caso o ficheiro nao esteja aberto ou nao tenha sido aberto em modo de escrita ou leitura/escrita. */
int tfsWrite(int fd, char *buffer, int len){
	int code;
	int size= strlen(buffer)+ 4;
	char tmpBuffer[len];

	if (sockfd==-1){
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	}

	if (len>MAX_STRING){
		return TECNICOFS_ERROR_OTHER;
	}

	tmpBuffer[len]='\0';
	strncpy(tmpBuffer,buffer,len);
	
	if (dprintf(sockfd, "w %d %s%s", fd, tmpBuffer, "\0") != size){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}
	int n = read(sockfd, &code, sizeof(int));

	if (n < 0){
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	}

	return code;
}
