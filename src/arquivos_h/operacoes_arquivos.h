/* operacoes_arquivos.h - struct do FCB (inode simulado) e prototipos das operacoes de arquivo (3.2) */
#ifndef OPERACOES_ARQUIVOS_H
#define OPERACOES_ARQUIVOS_H

#include <time.h>
#include "alocacao_blocos_disco.h"

#define MAX_NOME 32

typedef enum {
    TIPO_DIRETORIO,
    TIPO_NUMERICO,   /* extensao .num */
    TIPO_CARACTERE,  /* padrao (texto) */
    TIPO_BINARIO,    /* extensao .bin */
    TIPO_PROGRAMA    /* extensao .sh / .exe */
} TipoArquivo;

/* File Control Block (FCB) - "inode" simulado com todos os atributos do arquivo */
typedef struct {
    char        nome[MAX_NOME];
    int         id;                      /* numero do inode (unico) */
    TipoArquivo tipo;
    int         tamanho;                 /* em bytes */
    int         dono, grupo;             /* uid e gid */
    int         permissoes;              /* bitmask rwxrwxrwx (ex.: 0644) */
    time_t      criado, modificado, acessado;
    int         blocos[MAX_BLOCOS_ARQ];  /* bloco de indice (alocacao indexada) */
    int         num_blocos;
} FCB;

const char *tipo_texto(TipoArquivo t);

void cmd_touch(const char *nome);
void cmd_echo(const char *texto, const char *nome); /* echo texto > nome */
void cmd_cat(const char *nome);
void cmd_cp(const char *origem, const char *destino);
void cmd_mv(const char *origem, const char *destino);
void cmd_rm(const char *nome);
void cmd_stat(const char *nome);
void cmd_exec(const char *nome);                    /* "executar" (testa o bit X) */

#endif
