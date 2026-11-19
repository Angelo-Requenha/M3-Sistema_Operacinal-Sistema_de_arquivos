/* alocacao_blocos_disco.h - disco simulado (char[]) e tamanho do bloco (3.4) */
#ifndef ALOCACAO_BLOCOS_DISCO_H
#define ALOCACAO_BLOCOS_DISCO_H

#define TAM_BLOCO      16   /* bytes por bloco */
#define NUM_BLOCOS     64   /* total de blocos do disco */
#define MAX_BLOCOS_ARQ 8    /* entradas do bloco de indice de cada arquivo (max 128 bytes) */

/* Alocacao INDEXADA: cada FCB guarda um vetor com os numeros dos blocos
 * (o "bloco de indice"). Os blocos nao precisam ser contiguos no disco. */

int  disco_livres(void);                                        /* quantos blocos livres */
int  disco_gravar(const char *dados, int tamanho, int *blocos); /* aloca e grava; retorna n de blocos ou -1 */
void disco_ler(const int *blocos, int tamanho, char *saida);    /* junta os blocos em 'saida' (tamanho+1) */
void disco_liberar(int *blocos, int n);                         /* devolve os blocos ao bitmap */
void disco_mostrar(void);                                       /* comando df: mapa de blocos */

#endif
