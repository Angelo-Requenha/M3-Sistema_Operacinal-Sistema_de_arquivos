/* controle_acesso_permissoes.h - bits RWX (owner/group/others) e usuario atual (3.3) */
#ifndef CONTROLE_ACESSO_PERMISSOES_H
#define CONTROLE_ACESSO_PERMISSOES_H

/* Bits de uma classe. Owner usa os bits << 6, group << 3, others << 0.
 * Ex.: 0754 = rwx (owner) r-x (group) r-- (others) */
#define PERM_R 4
#define PERM_W 2
#define PERM_X 1

#define MAX_USUARIOS 16
#define MAX_GRUPOS   16
#define MAX_TEXTO    32  /* tamanho maximo de nome e senha */

typedef struct {
    char nome[MAX_TEXTO];
    char senha[MAX_TEXTO]; /* "" = usuario sem senha */
    int  uid;
    int  gid;              /* grupo do usuario */
} Usuario;

typedef struct {
    char nome[MAX_TEXTO];
    int  gid;
} Grupo;

extern Usuario usuario_atual; /* "usuario atual" global */

/* Retorna 1 se o usuario atual pode fazer 'op' (PERM_R/W/X); senao imprime
 * "Permissao negada" e retorna 0 (bloqueio obrigatorio). */
int  verificar_permissao(int perm, int dono, int grupo, int op, const char *nome);
void perm_texto(int perm, int eh_dir, char *saida); /* 0755 -> "drwxr-xr-x" */
const char *nome_usuario(int uid);
const char *nome_grupo(int gid);

void cmd_chmod(const char *modo, const char *nome);
void cmd_su(const char *nome);        /* pede senha se o usuario tiver uma */
void cmd_whoami(void);
void cmd_useradd(const char *nome, const char *grupo, const char *senha); /* so root */
void cmd_groupadd(const char *nome);                                      /* so root */
void cmd_groups(void);                /* lista grupos e seus membros */

#endif
