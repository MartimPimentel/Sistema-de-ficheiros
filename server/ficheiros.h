#ifndef FICHEIROS_H
#define FICHEIROS_H

typedef struct Fich_aberto {
    int inumber;
    int modoLeitura;
} Fich_aberto;

Fich_aberto* ficheiro_init();
void ficheiro_destroy(Fich_aberto *tabela_logs);
int isFileOpen(Fich_aberto *tabela_logs,int inumber);
int insertOpenFile(Fich_aberto *tabela_logs,int inumber,int mode);
int closeOpenedFile(Fich_aberto *tabela_logs,int inumber);

#endif /* FICHEIROS_H */