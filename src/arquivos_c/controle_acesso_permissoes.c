/* controle_acesso_permissoes.c - chmod numerico e checagem de permissao antes de cada operacao (3.3) */
#include <stdio.h>
#include <string.h>
#include "../arquivos_h/controle_acesso_permissoes.h"
#include "../arquivos_h/arvore_diretorios.h"

/* Usuarios iniciais. alice e bob estao no mesmo grupo (alunos).
 * O root tem a senha padrao "admin"; os demais nao tem senha.
 * Novos usuarios/grupos sao criados pelo root com useradd/groupadd. */
static Usuario usuarios[MAX_USUARIOS] = {
    {"root",   "admin", 0,    0},
    {"alice",  "",      1000, 100},
    {"bob",    "",      1001, 100},
    {"carlos", "",      1002, 200},
};
static int num_usuarios = 4;

static Grupo grupos[MAX_GRUPOS] = {
    {"root",       0},
    {"alunos",     100},
    {"visitantes", 200},
};
static int num_grupos = 3;

Usuario usuario_atual = {"alice", "", 1000, 100};

const char *nome_usuario(int uid) {
    int i;
    for (i = 0; i < num_usuarios; i++)
        if (usuarios[i].uid == uid) return usuarios[i].nome;
    return "?";
}

const char *nome_grupo(int gid) {
    int i;
    for (i = 0; i < num_grupos; i++)
        if (grupos[i].gid == gid) return grupos[i].nome;
    return "?";
}

static Usuario *buscar_usuario(const char *nome) {
    int i;
    for (i = 0; i < num_usuarios; i++)
        if (strcmp(usuarios[i].nome, nome) == 0) return &usuarios[i];
    return NULL;
}

static Grupo *buscar_grupo(const char *nome) {
    int i;
    for (i = 0; i < num_grupos; i++)
        if (strcmp(grupos[i].nome, nome) == 0) return &grupos[i];
    return NULL;
}

/* So o root pode gerenciar usuarios e grupos */
static int exigir_root(const char *cmd) {
    if (usuario_atual.uid == 0) return 1;
    printf("Permissao negada: apenas o root pode usar '%s'\n", cmd);
    return 0;
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

/* su: troca o usuario atual. Pede a senha se o usuario tiver uma
 * (como no Linux, o root troca para qualquer usuario sem senha). */
void cmd_su(const char *nome) {
    char senha[64];
    Usuario *u;

    if (!nome) { printf("Uso: su <usuario>\n"); return; }
    u = buscar_usuario(nome);
    if (!u) { printf("su: usuario '%s' nao existe (veja 'groups')\n", nome); return; }

    if (u->senha[0] && usuario_atual.uid != 0) {
        printf("Senha: ");
        fflush(stdout);
        if (!fgets(senha, sizeof(senha), stdin)) return;
        senha[strcspn(senha, "\r\n")] = '\0';
        if (strcmp(senha, u->senha) != 0) { printf("su: senha incorreta\n"); return; }
    }
    usuario_atual = *u;
}

void cmd_whoami(void) {
    printf("%s (uid=%d gid=%d %s)\n", usuario_atual.nome, usuario_atual.uid,
           usuario_atual.gid, nome_grupo(usuario_atual.gid));
}

/* useradd <nome> <grupo> [senha] : sem senha, o usuario fica sem senha */
void cmd_useradd(const char *nome, const char *grupo, const char *senha) {
    Grupo *g;
    Usuario *novo;

    if (!nome || !grupo) { printf("Uso: useradd <nome> <grupo> [senha]\n"); return; }
    if (!exigir_root("useradd")) return;
    if (strlen(nome) >= MAX_TEXTO || (senha && strlen(senha) >= MAX_TEXTO)) {
        printf("useradd: nome e senha devem ter no maximo %d caracteres\n", MAX_TEXTO - 1);
        return;
    }
    if (buscar_usuario(nome)) { printf("useradd: usuario '%s' ja existe\n", nome); return; }
    if (!(g = buscar_grupo(grupo))) { printf("useradd: grupo '%s' nao existe\n", grupo); return; }
    if (num_usuarios == MAX_USUARIOS) { printf("useradd: limite de %d usuarios\n", MAX_USUARIOS); return; }

    novo = &usuarios[num_usuarios];
    strcpy(novo->nome, nome);
    strcpy(novo->senha, senha ? senha : "");
    novo->uid = usuarios[num_usuarios - 1].uid + 1; /* proximo uid livre */
    novo->gid = g->gid;
    num_usuarios++;
    printf("Usuario '%s' criado (uid=%d, grupo %s)\n", nome, novo->uid, g->nome);
}

/* groupadd <nome> : gid gerado automaticamente (300, 400, ...) */
void cmd_groupadd(const char *nome) {
    Grupo *novo;

    if (!nome) { printf("Uso: groupadd <nome>\n"); return; }
    if (!exigir_root("groupadd")) return;
    if (strlen(nome) >= MAX_TEXTO) { printf("groupadd: nome muito longo\n"); return; }
    if (buscar_grupo(nome)) { printf("groupadd: grupo '%s' ja existe\n", nome); return; }
    if (num_grupos == MAX_GRUPOS) { printf("groupadd: limite de %d grupos\n", MAX_GRUPOS); return; }

    novo = &grupos[num_grupos];
    strcpy(novo->nome, nome);
    novo->gid = grupos[num_grupos - 1].gid + 100;
    num_grupos++;
    printf("Grupo '%s' criado (gid=%d)\n", nome, novo->gid);
}

/* groups: lista cada grupo com seus membros */
void cmd_groups(void) {
    int g, u;
    for (g = 0; g < num_grupos; g++) {
        printf("%-12s gid=%-4d:", grupos[g].nome, grupos[g].gid);
        for (u = 0; u < num_usuarios; u++)
            if (usuarios[u].gid == grupos[g].gid) printf(" %s", usuarios[u].nome);
        printf("\n");
    }
}
