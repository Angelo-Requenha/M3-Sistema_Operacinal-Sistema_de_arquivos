/* arvore_diretorios.c - mkdir, cd, ls e busca de caminhos na arvore (3.1) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arquivos_h/arvore_diretorios.h"
#include "../arquivos_h/controle_acesso_permissoes.h"

No *raiz  = NULL;
No *atual = NULL;
static int proximo_id = 1; /* gerador de numeros de inode */

/* Aloca um no novo com o FCB preenchido (dono = usuario atual) */
No *criar_no(const char *nome, TipoArquivo tipo) {
    No *no = calloc(1, sizeof(No));
    if (!no) { printf("Erro: memoria insuficiente\n"); return NULL; }

    strncpy(no->fcb.nome, nome, MAX_NOME - 1);
    no->fcb.id         = proximo_id++;
    no->fcb.tipo       = tipo;
    no->fcb.dono       = usuario_atual.uid;
    no->fcb.grupo      = usuario_atual.gid;
    no->fcb.permissoes = (tipo == TIPO_DIRETORIO) ? 0755 : 0644; /* como umask 022 */
    no->fcb.criado = no->fcb.modificado = no->fcb.acessado = time(NULL);
    return no;
}

void inserir_filho(No *dir, No *no) {
    no->pai   = dir;
    no->irmao = dir->filho;
    dir->filho = no;
}

void remover_filho(No *dir, No *no) {
    No **p = &dir->filho;
    while (*p && *p != no) p = &(*p)->irmao;
    if (*p) *p = no->irmao;
    no->pai = NULL;
    no->irmao = NULL;
}

/* Libera o no, seus filhos e os blocos de disco usados (evita memory leak) */
void liberar_no(No *no) {
    No *f = no->filho;
    while (f) {
        No *prox = f->irmao;
        liberar_no(f);
        f = prox;
    }
    disco_liberar(no->fcb.blocos, no->fcb.num_blocos);
    free(no);
}

No *buscar_filho(No *dir, const char *nome) {
    No *f;
    for (f = dir->filho; f; f = f->irmao)
        if (strcmp(f->fcb.nome, nome) == 0) return f;
    return NULL;
}

No *resolver(const char *nome) {
    if (strcmp(nome, "/") == 0)  return raiz;
    if (strcmp(nome, ".") == 0)  return atual;
    if (strcmp(nome, "..") == 0) return atual->pai ? atual->pai : raiz;
    return buscar_filho(atual, nome);
}

int nome_valido(const char *nome) {
    int len = (int)strlen(nome);
    if (len == 0 || len >= MAX_NOME || strchr(nome, '/') ||
        strcmp(nome, ".") == 0 || strcmp(nome, "..") == 0) {
        printf("Erro: nome invalido '%s' (1 a %d caracteres, sem '/', '.' ou '..')\n",
               nome, MAX_NOME - 1);
        return 0;
    }
    return 1;
}

/* Monta o caminho absoluto subindo pelos ponteiros 'pai' */
void caminho(No *no, char *buf, int tam) {
    if (no == raiz) { snprintf(buf, tam, "/"); return; }
    caminho(no->pai, buf, tam);
    if (no->pai != raiz) strncat(buf, "/", tam - strlen(buf) - 1);
    strncat(buf, no->fcb.nome, tam - strlen(buf) - 1);
}

void arvore_iniciar(void) {
    Usuario salvo = usuario_atual;
    usuario_atual.uid = 0;                  /* "/" pertence ao root */
    usuario_atual.gid = 0;
    raiz = criar_no("/", TIPO_DIRETORIO);
    raiz->fcb.permissoes = 0777;            /* todos podem criar na raiz */
    usuario_atual = salvo;
    atual = raiz;
}

void arvore_destruir(void) {
    liberar_no(raiz);
    raiz = atual = NULL;
}

void cmd_mkdir(const char *nome) {
    No *no;
    if (!nome) { printf("Uso: mkdir <nome>\n"); return; }
    if (!nome_valido(nome)) return;
    if (buscar_filho(atual, nome)) { printf("mkdir: '%s' ja existe\n", nome); return; }
    if (!verificar_permissao(atual->fcb.permissoes, atual->fcb.dono, atual->fcb.grupo,
                             PERM_W, atual->fcb.nome)) return;
    no = criar_no(nome, TIPO_DIRETORIO);
    if (no) inserir_filho(atual, no);
}

void cmd_cd(const char *nome) {
    No *no = nome ? resolver(nome) : raiz;
    if (!no) { printf("cd: '%s' nao encontrado\n", nome); return; }
    if (no->fcb.tipo != TIPO_DIRETORIO) { printf("cd: '%s' nao e um diretorio\n", nome); return; }
    if (!verificar_permissao(no->fcb.permissoes, no->fcb.dono, no->fcb.grupo,
                             PERM_X, no->fcb.nome)) return;
    atual = no;
}

/* ls no formato do "ls -l" do Linux, com o inode na primeira coluna (ls -li) */
void cmd_ls(void) {
    No *f;
    char perm[11], data[20];

    if (!verificar_permissao(atual->fcb.permissoes, atual->fcb.dono, atual->fcb.grupo,
                             PERM_R, atual->fcb.nome)) return;

    for (f = atual->filho; f; f = f->irmao) {
        perm_texto(f->fcb.permissoes, f->fcb.tipo == TIPO_DIRETORIO, perm);
        strftime(data, sizeof(data), "%d/%m/%Y %H:%M", localtime(&f->fcb.modificado));
        printf("%3d %s %-7s %-10s %5d %s %s%s\n", f->fcb.id, perm, nome_usuario(f->fcb.dono),
               nome_grupo(f->fcb.grupo), f->fcb.tamanho, data, f->fcb.nome,
               f->fcb.tipo == TIPO_DIRETORIO ? "/" : "");
    }
}

void cmd_pwd(void) {
    char buf[512];
    caminho(atual, buf, sizeof(buf));
    printf("%s\n", buf);
}
