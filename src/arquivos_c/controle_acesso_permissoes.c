/* controle_acesso_permissoes.c - chmod numerico e checagem de permissao antes de cada operacao (3.3) */
#include <stdio.h>
#include <string.h>
#include "../arquivos_h/controle_acesso_permissoes.h"
#include "../arquivos_h/arvore_diretorios.h"

/* Usuarios fixos do simulador. alice e bob estao no mesmo grupo (100). */
static Usuario usuarios[] = {
    {"root",   0,    0},
    {"alice",  1000, 100},
    {"bob",    1001, 100},
    {"carlos", 1002, 200},
};
#define NUM_USUARIOS (int)(sizeof(usuarios) / sizeof(usuarios[0]))

Usuario usuario_atual = {"alice", 1000, 100};

const char *nome_usuario(int uid) {
    int i;
    for (i = 0; i < NUM_USUARIOS; i++)
        if (usuarios[i].uid == uid) return usuarios[i].nome;
    return "?";
}

/* Escolhe a classe (owner, group ou others) e testa o bit com AND bit a bit */
int verificar_permissao(int perm, int dono, int grupo, int op, const char *nome) {
    int desloc;

    if (usuario_atual.uid == 0) return 1;            /* root pode tudo */

    if (usuario_atual.uid == dono)       desloc = 6; /* owner  */
    else if (usuario_atual.gid == grupo) desloc = 3; /* group  */
    else                                 desloc = 0; /* others */

    if ((perm >> desloc) & op) return 1;

    printf("Permissao negada: '%s' (precisa de %c)\n", nome,
           op == PERM_R ? 'r' : op == PERM_W ? 'w' : 'x');
    return 0;
}

void perm_texto(int perm, int eh_dir, char *saida) {
    const char *letras = "rwxrwxrwx";
    int i;
    saida[0] = eh_dir ? 'd' : '-';
    for (i = 0; i < 9; i++)
        saida[i + 1] = (perm & (1 << (8 - i))) ? letras[i] : '-';
    saida[10] = '\0';
}

/* chmod numerico (octal): ex. "chmod 640 arq" */
void cmd_chmod(const char *modo, const char *nome) {
    No *no;
    int i, valor = 0;

    if (!modo || !nome) { printf("Uso: chmod <octal> <nome>\n"); return; }

    for (i = 0; modo[i]; i++) {
        if (modo[i] < '0' || modo[i] > '7' || i >= 3) {
            printf("chmod: modo invalido '%s' (use 3 digitos octais, ex.: 755)\n", modo);
            return;
        }
        valor = (valor << 3) | (modo[i] - '0'); /* cada digito octal = 3 bits */
    }

    no = resolver(nome);
    if (!no) { printf("chmod: '%s' nao encontrado\n", nome); return; }

    if (usuario_atual.uid != 0 && usuario_atual.uid != no->fcb.dono) {
        printf("Permissao negada: apenas o dono ou root pode alterar '%s'\n", nome);
        return;
    }

    no->fcb.permissoes = valor;
    no->fcb.modificado = time(NULL);
}

void cmd_su(const char *nome) {
    int i;
    if (!nome) { printf("Uso: su <usuario>\n"); return; }
    for (i = 0; i < NUM_USUARIOS; i++) {
        if (strcmp(usuarios[i].nome, nome) == 0) {
            usuario_atual = usuarios[i];
            return;
        }
    }
    printf("su: usuario '%s' nao existe (root, alice, bob, carlos)\n", nome);
}

void cmd_whoami(void) {
    printf("%s (uid=%d gid=%d)\n", usuario_atual.nome, usuario_atual.uid, usuario_atual.gid);
}
