#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "ficheiros.h"
#include "../client/tecnicofs-api-constants.h"

#define ARRAY_ENTRADAS 5

/*Inicializacao da tabela de ficheiros*/
Fich_aberto* ficheiro_init(){

    Fich_aberto *tabela_logs = (Fich_aberto*)malloc(sizeof(Fich_aberto)*(ARRAY_ENTRADAS + 1));
    for (int i = 0; i < ARRAY_ENTRADAS; i++){
    	tabela_logs[i].inumber = -1;
    	tabela_logs[i].modoLeitura = -1;
    }
    return tabela_logs;
}

/*Destroi a tabela da ficheiros*/
void ficheiro_destroy(Fich_aberto *tabela_logs){
	free(tabela_logs);
}

/*Verifica se o ficheiro esta aberto. Se sim, retorna o seu index na tabela.*/
int isFileOpen(Fich_aberto *tabela_logs, int inumber){
	
	for (int i = 0; i < ARRAY_ENTRADAS; i++){
		if (tabela_logs[i].inumber == inumber){
			return i;
		}
	}
	return -1;
}

/*Insere um ficheiro,se este ainda nao estiver aberto.*/
int insertOpenFile(Fich_aberto *tabela_logs, int inumber, int mode){

	if (isFileOpen(tabela_logs, inumber) != -1){
		return TECNICOFS_ERROR_FILE_IS_OPEN;
	}
	for (int i = 0; i < ARRAY_ENTRADAS; i++){
		if (tabela_logs[i].inumber == -1){
			tabela_logs[i].inumber = inumber;
			tabela_logs[i].modoLeitura = mode;
			return i;
		}
	}
	return TECNICOFS_ERROR_MAXED_OPEN_FILES;
}

/*Fecha um ficheiro,se este estiver aberto.*/
int closeOpenedFile(Fich_aberto *tabela_logs, int inumber){
	int slot = isFileOpen(tabela_logs, inumber);

	if (slot == -1){
		return TECNICOFS_ERROR_FILE_NOT_OPEN;
	}
	tabela_logs[slot].inumber = -1;
	tabela_logs[slot].modoLeitura = -1;

	return 0;
}
