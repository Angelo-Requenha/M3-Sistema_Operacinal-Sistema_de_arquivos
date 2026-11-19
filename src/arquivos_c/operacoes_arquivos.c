/* operacoes_arquivos.c - touch, echo >, cat, cp, mv, rm (3.2) */
#include <stdio.h>
#include <string.h>
#include "../arquivos_h/operacoes_arquivos.h"
#include "../arquivos_h/arvore_diretorios.h"
#include "../arquivos_h/controle_acesso_permissoes.h"

#define MAX_CONTEUDO (MAX_BLOCOS_ARQ * TAM_BLOCO)

const char *tipo_texto(TipoArquivo t) {
    switch (t) {
        case TIPO_DIRETORIO: return "diretorio";
        case TIPO_NUMERICO:  return "numerico";
        case TIPO_CARACTERE: return "caractere";
        case TIPO_BINARIO:   return "binario";
        case TIPO_PROGRAMA:  return "programa";
    }
    return "?";
}

/* O tipo do arquivo e definido pela extensao do nome */
static TipoArquivo tipo_por_nome(const char *nome) {
    const char *ext = strrchr(nome, '.');
    if (!ext) return TIPO_CARACTERE;
    if (strcmp(ext, ".num") == 0) return TIPO_NUMERICO;
    if (strcmp(ext, ".bin") == 0) return TIPO_BINARIO;
    if (strcmp(ext, ".sh") == 0 || strcmp(ext, ".exe") == 0) return TIPO_PROGRAMA;
    return TIPO_CARACTERE;
}

/* Atalho: verifica a permissao 'op' do usuario atual sobre o no */
static int pode(No *no, int op) {
    return verificar_permissao(no->fcb.permissoes, no->fcb.dono, no->fcb.grupo, op, no->fcb.nome);
}

/* Busca um ARQUIVO (nao diretorio) no diretorio atual, com mensagem de erro */
static No *achar_arquivo(const char *cmd, const char *nome) {
    No *no;
    if (!nome) { printf("Uso: %s <nome>\n", cmd); return NULL; }
    no = buscar_filho(atual, nome);
    if (!no) { printf("%s: '%s' nao encontrado\n", cmd, nome); return NULL; }
    if (no->fcb.tipo == TIPO_DIRETORIO) { printf("%s: '%s' e um diretorio\n", cmd, nome); return NULL; }
    return no;
}

/* Cria um arquivo vazio em 'dir' (exige permissao W no diretorio) */
static No *criar_arquivo(No *dir, const char *nome) {
    No *no;
    if (!nome_valido(nome) || !pode(dir, PERM_W)) return NULL;
    no = criar_no(nome, tipo_por_nome(nome));
    if (no) inserir_filho(dir, no);
    return no;
}

/* Substitui o conteudo do arquivo: libera os blocos antigos e aloca novos */
static int gravar_conteudo(No *f, const char *dados, int tamanho) {
    int n = (tamanho + TAM_BLOCO - 1) / TAM_BLOCO;

    if (n > MAX_BLOCOS_ARQ) {
        printf("Erro: arquivo muito grande (max %d bytes)\n", MAX_CONTEUDO);
        return 0;
    }
    if (n > disco_livres() + f->fcb.num_blocos) {
        printf("Erro: disco cheio\n");
        return 0;
    }

    disco_liberar(f->fcb.blocos, f->fcb.num_blocos);
    f->fcb.num_blocos = disco_gravar(dados, tamanho, f->fcb.blocos);
    f->fcb.tamanho    = tamanho;
    f->fcb.modificado = f->fcb.acessado = time(NULL);
    return 1;
}

void cmd_touch(const char *nome) {
    No *no;
    if (!nome) { printf("Uso: touch <nome>\n"); return; }
    no = buscar_filho(atual, nome);
    if (no) {                                   /* ja existe: so atualiza as datas */
        if (pode(no, PERM_W)) no->fcb.modificado = no->fcb.acessado = time(NULL);
        return;
    }
    criar_arquivo(atual, nome);
}

void cmd_echo(const char *texto, const char *nome) {
    No *no;
    if (!nome) { printf("Uso: echo <texto> > <nome>\n"); return; }

    no = buscar_filho(atual, nome);
    if (no && no->fcb.tipo == TIPO_DIRETORIO) { printf("echo: '%s' e um diretorio\n", nome); return; }
    if (no) { if (!pode(no, PERM_W)) return; }
    else if (!(no = criar_arquivo(atual, nome))) return;

    gravar_conteudo(no, texto, (int)strlen(texto));
}

void cmd_cat(const char *nome) {
    char buf[MAX_CONTEUDO + 1];
    No *no = achar_arquivo("cat", nome);
    if (!no || !pode(no, PERM_R)) return;

    disco_ler(no->fcb.blocos, no->fcb.tamanho, buf);
    printf("%s\n", buf);
    no->fcb.acessado = time(NULL);
}

/* cp origem destino   (destino pode ser um nome novo/existente ou um diretorio) */
void cmd_cp(const char *origem, const char *destino) {
    char buf[MAX_CONTEUDO + 1];
    No *src, *dir = atual, *dst;
    const char *nome = destino;

    if (!origem || !destino) { printf("Uso: cp <origem> <destino>\n"); return; }
    if (!(src = achar_arquivo("cp", origem)) || !pode(src, PERM_R)) return;

    dst = resolver(destino);
    if (dst && dst->fcb.tipo == TIPO_DIRETORIO) {  /* copiar para dentro do diretorio */
        dir  = dst;
        nome = src->fcb.nome;
        dst  = buscar_filho(dir, nome);
    }

    if (dst == src) { printf("cp: origem e destino sao o mesmo arquivo\n"); return; }
    if (dst && dst->fcb.tipo == TIPO_DIRETORIO) { printf("cp: '%s' e um diretorio\n", nome); return; }
    if (dst) { if (!pode(dst, PERM_W)) return; }
    else if (!(dst = criar_arquivo(dir, nome))) return;

    disco_ler(src->fcb.blocos, src->fcb.tamanho, buf);
    gravar_conteudo(dst, buf, src->fcb.tamanho);
    src->fcb.acessado = time(NULL);
}

/* mv origem destino   (se destino for diretorio, move; senao, renomeia) */
void cmd_mv(const char *origem, const char *destino) {
    No *src, *dst;

    if (!origem || !destino) { printf("Uso: mv <origem> <destino>\n"); return; }
    src = buscar_filho(atual, origem);
    if (!src) { printf("mv: '%s' nao encontrado\n", origem); return; }
    if (!pode(atual, PERM_W)) return;

    dst = resolver(destino);
    if (dst && dst->fcb.tipo == TIPO_DIRETORIO) {  /* mover para outro diretorio */
        if (dst == src) { printf("mv: nao e possivel mover '%s' para dentro dele mesmo\n", origem); return; }
        if (buscar_filho(dst, src->fcb.nome)) { printf("mv: '%s' ja existe no destino\n", src->fcb.nome); return; }
        if (!pode(dst, PERM_W)) return;
        remover_filho(atual, src);
        inserir_filho(dst, src);
        return;
    }
    if (dst) { printf("mv: '%s' ja existe\n", destino); return; }
    if (!nome_valido(destino)) return;

    strncpy(src->fcb.nome, destino, MAX_NOME - 1);  /* renomear = trocar o nome no FCB */
    src->fcb.nome[MAX_NOME - 1] = '\0';
}

void cmd_rm(const char *nome) {
    No *no;
    if (!nome) { printf("Uso: rm <nome>\n"); return; }
    no = buscar_filho(atual, nome);
    if (!no) { printf("rm: '%s' nao encontrado\n", nome); return; }
    if (no->filho) { printf("rm: diretorio '%s' nao esta vazio\n", nome); return; }
    if (!pode(atual, PERM_W)) return;

    remover_filho(atual, no);
    liberar_no(no);  /* libera o FCB e devolve os blocos ao disco */
}

/* Mostra o FCB (inode) completo e como o conteudo esta dividido em blocos */
void cmd_stat(const char *nome) {
    char perm[11], buf[MAX_CONTEUDO + 1];
    int i;
    No *no;

    if (!nome) { printf("Uso: stat <nome>\n"); return; }
    no = resolver(nome);
    if (!no) { printf("stat: '%s' nao encontrado\n", nome); return; }

    perm_texto(no->fcb.permissoes, no->fcb.tipo == TIPO_DIRETORIO, perm);
    printf("  Nome: %s\n", no->fcb.nome);
    printf(" Inode: %d   Tipo: %s   Tamanho: %d bytes\n", no->fcb.id, tipo_texto(no->fcb.tipo), no->fcb.tamanho);
    printf("Acesso: (%04o/%s)  Dono: %s (uid %d)  Grupo: %s (gid %d)\n", no->fcb.permissoes, perm,
           nome_usuario(no->fcb.dono), no->fcb.dono, nome_grupo(no->fcb.grupo), no->fcb.grupo);
    printf(" Criado: %s", ctime(&no->fcb.criado));
    printf("Modific: %s", ctime(&no->fcb.modificado));
    printf(" Acesso: %s", ctime(&no->fcb.acessado));

    printf("Blocos (indice): %d ->", no->fcb.num_blocos);
    for (i = 0; i < no->fcb.num_blocos; i++) printf(" %d", no->fcb.blocos[i]);
    printf("\n");

    /* Conteudo de cada bloco (precisa de permissao de leitura) */
    if (no->fcb.num_blocos > 0 && pode(no, PERM_R)) {
        disco_ler(no->fcb.blocos, no->fcb.tamanho, buf);
        for (i = 0; i < no->fcb.num_blocos; i++)
            printf("  bloco %2d: \"%.*s\"\n", no->fcb.blocos[i], TAM_BLOCO, buf + i * TAM_BLOCO);
    }
}

/* "Executa" o arquivo: testa o bit X e mostra o conteudo do programa */
void cmd_exec(const char *nome) {
    char buf[MAX_CONTEUDO + 1];
    No *no = achar_arquivo("exec", nome);
    if (!no || !pode(no, PERM_X)) return;
    if (no->fcb.tipo != TIPO_PROGRAMA) { printf("exec: '%s' nao e um programa (.sh/.exe)\n", nome); return; }

    disco_ler(no->fcb.blocos, no->fcb.tamanho, buf);
    printf("[executando %s] %s\n", nome, buf);
    no->fcb.acessado = time(NULL);
}
