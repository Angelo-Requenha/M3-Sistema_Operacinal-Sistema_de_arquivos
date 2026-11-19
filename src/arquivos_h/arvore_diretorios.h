/* arvore_diretorios.h - struct do no de diretorio (arvore) e prototipos de mkdir, cd, ls (3.1) */
#ifndef ARVORE_DIRETORIOS_H
#define ARVORE_DIRETORIOS_H

#include "operacoes_arquivos.h"

/* No da arvore. Arquivos e diretorios sao nos; so diretorios tem filhos.
 * Representacao "primeiro filho / proximo irmao" (lista encadeada de filhos). */
typedef struct No {
    FCB        fcb;
    struct No *pai;
    struct No *filho;  /* primeiro filho */
    struct No *irmao;  /* proximo no do mesmo diretorio */
} No;

extern No *raiz;
extern No *atual; /* diretorio corrente */

void arvore_iniciar(void);
void arvore_destruir(void);

No  *criar_no(const char *nome, TipoArquivo tipo);
void inserir_filho(No *dir, No *no);
void remover_filho(No *dir, No *no);  /* so desliga da lista, nao libera */
void liberar_no(No *no);              /* libera recursivamente (e os blocos do disco) */
No  *buscar_filho(No *dir, const char *nome);
No  *resolver(const char *nome);      /* "/", ".", ".." ou nome no diretorio atual */
int  nome_valido(const char *nome);
void caminho(No *no, char *buf, int tam);

void cmd_mkdir(const char *nome);
void cmd_cd(const char *nome);
void cmd_ls(void);
void cmd_pwd(void);

#endif
