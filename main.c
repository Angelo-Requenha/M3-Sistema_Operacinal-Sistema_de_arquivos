/* UNIVALI - Sistemas Operacionais - Avaliacao M3
 * Alunos: Angelo Miguel Requenha, Bruno de Queiroz, Lucas Henrique de Melo Gubert
 *
 * main.c - ponto de entrada: loop do terminal que le o comando e chama a funcao certa (5.2) */
#include <stdio.h>
#include <string.h>
#include "src/arquivos_h/arvore_diretorios.h"
#include "src/arquivos_h/operacoes_arquivos.h"
#include "src/arquivos_h/controle_acesso_permissoes.h"
#include "src/arquivos_h/alocacao_blocos_disco.h"

static void ajuda(void) {
    printf("Comandos:\n"
           "  mkdir <dir>            cd <dir|..|/>        ls        pwd\n"
           "  touch <arq>            echo <texto> > <arq> cat <arq>\n"
           "  cp <orig> <dest>       mv <orig> <dest>     rm <arq|dir vazio>\n"
           "  chmod <octal> <nome>   stat <nome>          exec <arq.sh>\n"
           "  su <usuario>           whoami               df (mapa do disco)\n"
           "  groups (lista grupos e usuarios)\n"
           "  useradd <nome> <grupo> [senha]   groupadd <nome>   (apenas root)\n"
           "  help                   exit\n"
           "Usuarios iniciais: root (senha: admin), alice e bob (grupo alunos),\n"
           "carlos (grupo visitantes)\n");
}

/* Remove espacos do inicio/fim (modifica a string) */
static char *aparar(char *s) {
    char *fim;
    while (*s == ' ' || *s == '\t') s++;
    fim = s + strlen(s);
    while (fim > s && (fim[-1] == ' ' || fim[-1] == '\t')) *--fim = '\0';
    return s;
}

/* echo texto > arquivo  (o texto pode ter espacos e aspas) */
static void tratar_echo(char *resto) {
    char *seta = strrchr(resto, '>');
    char *texto, *nome;
    size_t n;

    if (!seta) { printf("%s\n", aparar(resto)); return; } /* echo sem '>' so imprime */
    *seta = '\0';
    texto = aparar(resto);
    nome  = aparar(seta + 1);

    n = strlen(texto);
    if (n >= 2 && texto[0] == '"' && texto[n - 1] == '"') { texto[n - 1] = '\0'; texto++; }

    cmd_echo(texto, *nome ? nome : NULL);
}

int main(void) {
    char linha[512], cwd[512];

    arvore_iniciar();
    printf("Mini-Sistema de Arquivos em Memoria - digite 'help'\n");

    for (;;) {
        char *cmd, *a1, *a2, *a3;

        caminho(atual, cwd, sizeof(cwd));
        printf("%s@fs:%s$ ", usuario_atual.nome, cwd);
        fflush(stdout);
        if (!fgets(linha, sizeof(linha), stdin)) break;
        linha[strcspn(linha, "\r\n")] = '\0';

        if (strncmp(linha, "echo ", 5) == 0) { tratar_echo(linha + 5); continue; }

        cmd = strtok(linha, " \t");
        a1  = strtok(NULL, " \t");
        a2  = strtok(NULL, " \t");
        a3  = strtok(NULL, " \t");
        if (!cmd) continue;

        if      (strcmp(cmd, "exit")   == 0) break;
        else if (strcmp(cmd, "help")   == 0) ajuda();
        else if (strcmp(cmd, "mkdir")  == 0) cmd_mkdir(a1);
        else if (strcmp(cmd, "cd")     == 0) cmd_cd(a1);
        else if (strcmp(cmd, "ls")     == 0) cmd_ls();
        else if (strcmp(cmd, "pwd")    == 0) cmd_pwd();
        else if (strcmp(cmd, "touch")  == 0) cmd_touch(a1);
        else if (strcmp(cmd, "cat")    == 0) cmd_cat(a1);
        else if (strcmp(cmd, "cp")     == 0) cmd_cp(a1, a2);
        else if (strcmp(cmd, "mv")     == 0) cmd_mv(a1, a2);
        else if (strcmp(cmd, "rm")     == 0) cmd_rm(a1);
        else if (strcmp(cmd, "chmod")  == 0) cmd_chmod(a1, a2);
        else if (strcmp(cmd, "stat")   == 0) cmd_stat(a1);
        else if (strcmp(cmd, "exec")   == 0) cmd_exec(a1);
        else if (strcmp(cmd, "su")     == 0) cmd_su(a1);
        else if (strcmp(cmd, "whoami") == 0) cmd_whoami();
        else if (strcmp(cmd, "groups") == 0) cmd_groups();
        else if (strcmp(cmd, "useradd")  == 0) cmd_useradd(a1, a2, a3);
        else if (strcmp(cmd, "groupadd") == 0) cmd_groupadd(a1);
        else if (strcmp(cmd, "df")     == 0) disco_mostrar();
        else printf("Comando desconhecido: '%s' (digite 'help')\n", cmd);
    }

    arvore_destruir(); /* libera toda a arvore e os blocos */
    return 0;
}
