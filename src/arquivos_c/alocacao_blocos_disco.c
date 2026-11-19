/* alocacao_blocos_disco.c - alocar/liberar blocos e ligar os blocos ao FCB (3.4) */
#include <stdio.h>
#include <string.h>
#include "../arquivos_h/alocacao_blocos_disco.h"

static char disco[NUM_BLOCOS * TAM_BLOCO]; /* o "disco" simulado */
static int  ocupado[NUM_BLOCOS];           /* bitmap de blocos livres (0) / ocupados (1) */

int disco_livres(void) {
    int i, n = 0;
    for (i = 0; i < NUM_BLOCOS; i++)
        if (!ocupado[i]) n++;
    return n;
}

/* Divide 'dados' em pedacos de TAM_BLOCO e grava cada um no primeiro bloco livre.
 * Os numeros dos blocos ficam em 'blocos' (bloco de indice do FCB). */
int disco_gravar(const char *dados, int tamanho, int *blocos) {
    int n = (tamanho + TAM_BLOCO - 1) / TAM_BLOCO; /* teto(tamanho / TAM_BLOCO) */
    int i, b = 0;

    if (n > MAX_BLOCOS_ARQ || n > disco_livres()) return -1;

    for (i = 0; i < n; i++) {
        int resto = tamanho - i * TAM_BLOCO;
        while (ocupado[b]) b++;                  /* procura o proximo bloco livre */
        ocupado[b] = 1;
        blocos[i] = b;
        memcpy(&disco[b * TAM_BLOCO], dados + i * TAM_BLOCO,
               resto < TAM_BLOCO ? resto : TAM_BLOCO);
    }
    return n;
}

/* Le os blocos na ordem do indice e remonta o conteudo */
void disco_ler(const int *blocos, int tamanho, char *saida) {
    int i, n = (tamanho + TAM_BLOCO - 1) / TAM_BLOCO;
    for (i = 0; i < n; i++) {
        int resto = tamanho - i * TAM_BLOCO;
        memcpy(saida + i * TAM_BLOCO, &disco[blocos[i] * TAM_BLOCO],
               resto < TAM_BLOCO ? resto : TAM_BLOCO);
    }
    saida[tamanho] = '\0';
}

void disco_liberar(int *blocos, int n) {
    int i;
    for (i = 0; i < n; i++) {
        ocupado[blocos[i]] = 0;
        memset(&disco[blocos[i] * TAM_BLOCO], 0, TAM_BLOCO);
    }
}

/* Mostra o mapa do disco: '#' = ocupado, '.' = livre */
void disco_mostrar(void) {
    int i;
    printf("Disco: %d blocos de %d bytes | livres: %d\n", NUM_BLOCOS, TAM_BLOCO, disco_livres());
    for (i = 0; i < NUM_BLOCOS; i++) {
        if (i % 16 == 0) printf("%3d: ", i);
        putchar(ocupado[i] ? '#' : '.');
        if (i % 16 == 15) putchar('\n');
    }
}
