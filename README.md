<p align="center">
  <b>UNIVERSIDADE DO VALE DO ITAJAÍ – UNIVALI</b><br>
  ESCOLA POLITÉCNICA<br>
  DISCIPLINA: SISTEMAS OPERACIONAIS<br>
  Avaliação M3 – Sistemas de Arquivos
</p>

<p align="center">
  <b>Alunos:</b><br>
  ANGELO MIGUEL REQUENHA<br>
  BRUNO DE QUEIROZ<br>
  LUCAS HENRIQUE DE MELO GUBERT
</p>

<p align="center">ITAJAÍ – 2026/2</p>

---

# Mini-Sistema de Arquivos em Memória (C)

Simulador de um sistema de arquivos estilo Unix que roda inteiramente em memória: árvore de diretórios, FCB/inode, permissões RWX e um disco simulado com alocação indexada de blocos. Nada é gravado no sistema de arquivos real.

## Como compilar e executar

Linux / WSL:

```bash
gcc -Wall -std=c99 -o fs main.c src/arquivos_c/*.c
./fs
```

Windows (MinGW/gcc no PowerShell):

```powershell
gcc -Wall -std=c99 -o fs.exe main.c src/arquivos_c/alocacao_blocos_disco.c src/arquivos_c/arvore_diretorios.c src/arquivos_c/controle_acesso_permissoes.c src/arquivos_c/operacoes_arquivos.c
.\fs.exe
```

Dentro do simulador, digite `help` para ver os comandos:

| Comando                                                     | O que faz                                        |
| ----------------------------------------------------------- | ------------------------------------------------ |
| `mkdir <dir>` / `cd <dir\|..\|/>` / `ls` / `pwd`      | diretórios (3.1)                                |
| `touch <arq>` / `echo <texto> > <arq>` / `cat <arq>`  | criar, escrever, ler (3.2)                       |
| `cp <orig> <dest>` / `mv <orig> <dest>` / `rm <nome>` | copiar, mover/renomear, excluir (3.2)            |
| `chmod <octal> <nome>` / `su <usuario>` / `whoami`    | permissões e usuário atual (3.3)               |
| `useradd <nome> <grupo> [senha]` / `groupadd <nome>` / `groups` | criar usuário e grupo (só root) / listar grupos (3.3) |
| `exec <arq.sh>`                                           | "executa" um programa (testa o bit X) (3.3)      |
| `stat <nome>` / `df`                                    | mostra o FCB com os blocos / mapa do disco (3.4) |

Usuários iniciais: `root` (uid 0, senha `admin`), `alice` (uid 1000) e `bob` (uid 1001) no grupo `alunos` (gid 100), e `carlos` (uid 1002) no grupo `visitantes` (gid 200). O simulador começa como `alice`.

O root pode criar grupos (`groupadd professores`, gid gerado automaticamente: 300, 400...) e usuários (`useradd michael professores 1234`). A senha é opcional: sem ela, o usuário fica sem senha. O `su` só pede senha quando o usuário de destino tem uma, e o root troca para qualquer usuário sem senha, como no Linux.

## Estruturas de dados e escolhas de design

```
main.c                                  loop do terminal (lê a linha e chama o comando)
src/arquivos_h|c/arvore_diretorios      struct No, mkdir, cd, ls, pwd           (3.1)
src/arquivos_h|c/operacoes_arquivos     struct FCB, touch, echo, cat, cp, mv, rm (3.2)
src/arquivos_h|c/controle_acesso_...    bits RWX, usuário atual, chmod, su      (3.3)
src/arquivos_h|c/alocacao_blocos_disco  disco char[], bitmap, alocação indexada (3.4)
```

**FCB** (`operacoes_arquivos.h`): `struct` com nome, id (inode), tipo (`enum TipoArquivo`), tamanho, dono, grupo, permissões (bitmask), datas de criação/modificação/acesso (`time_t`) e o vetor `blocos[]` (bloco de índice).

**Nó da árvore** (`arvore_diretorios.h`): cada arquivo ou diretório é um `No` alocado com `malloc` (`calloc`), contendo seu FCB e três ponteiros:

```c
typedef struct No {
    FCB fcb;
    struct No *pai;    /* para "cd .." e para montar o caminho no pwd */
    struct No *filho;  /* primeiro item do diretório */
    struct No *irmao;  /* próximo item do mesmo diretório */
} No;
```

Essa representação "primeiro filho / próximo irmão" transforma a árvore genérica numa estrutura com só dois ponteiros por nó; os itens de um diretório formam uma lista encadeada. Duas variáveis globais guardam a raiz (`/`) e o diretório atual.

**Memória:** `rm` desliga o nó da lista do pai, devolve os blocos ao disco e chama `free`. No `exit`, `arvore_destruir()` percorre a árvore recursivamente liberando tudo, sem memory leak.

## Conceitos teóricos

### Arquivo e atributos

Um arquivo é um tipo abstrato de dados identificado pelos seus atributos. No simulador eles ficam todos no FCB: **nome**, **identificador** (inode), **tipo** (numérico `.num`, caractere (padrão), binário `.bin`, programa `.sh`/`.exe`), **localização** (os blocos), **tamanho**, **proteção** (dono, grupo, permissões) e **datas** (criação, modificação, último acesso). O comando `stat` mostra todos eles.

### Operações com arquivos

| Operação teórica | Comando               | Implementação                                                                                                |
| ------------------- | --------------------- | -------------------------------------------------------------------------------------------------------------- |
| Criar               | `touch`, `echo >` | aloca um `No` com FCB novo e insere na lista do diretório                                                   |
| Escrever / truncar  | `echo texto > arq`  | libera os blocos antigos e grava o novo conteúdo (sobrescreve)                                                |
| Ler                 | `cat`               | remonta o conteúdo seguindo o índice de blocos; atualiza a data de acesso                                    |
| Copiar              | `cp`                | lê a origem e grava num FCB novo, com blocos próprios                                                        |
| Mover / renomear    | `mv`                | renomear só troca o nome no FCB; mover religa o nó em outro diretório (os blocos não mudam, como no Linux) |
| Excluir             | `rm`                | remove da árvore, libera os blocos e o nó                                                                    |

### FCB e inode simulado

O FCB é o "inode": guarda **tudo sobre o arquivo, menos o conteúdo**, e aponta para onde o conteúdo está (`blocos[]`). O `id` é único e gerado por um contador global, assim como o número de inode no Linux (`ls -i`). O nome também fica no FCB por simplicidade (no Linux real ele fica na entrada de diretório).

### Diretório em árvore

- **Eficiência:** a busca é feita só dentro do diretório atual, não em todos os arquivos do sistema.
- **Nomeação:** dois arquivos podem ter o mesmo nome em diretórios diferentes (`/a/x` e `/b/x`); o caminho absoluto é montado subindo pelo ponteiro `pai` (`pwd`).
- **Agrupamento:** os arquivos podem ser organizados por assunto ou usuário em subdiretórios (`mkdir`, `mv arq dir`).

### Proteção (RWX e chmod com bitmask)

As permissões são um inteiro de 9 bits: `rwx` do dono, `rwx` do grupo e `rwx` dos outros. Ex.: `0754` = `111 101 100` = `rwxr-xr--`.

- `chmod 640 arq` lê cada dígito octal como 3 bits: `valor = (valor << 3) | digito`. Só o dono ou o root podem alterar.
- Para verificar, o simulador escolhe a classe do usuário atual (dono → desloca 6, mesmo grupo → desloca 3, outros → 0) e testa o bit com AND: `(perm >> desloc) & PERM_R`.
- **Bloqueio obrigatório:** todas as operações passam por `verificar_permissao` antes de executar; se o bit não estiver ligado, a operação é negada com `Permissao negada`. O root ignora as permissões.
- **Usuários e grupos:** ficam em dois vetores (`usuarios[]` e `grupos[]`) com um contador cada, em `controle_acesso_permissoes.c`. A struct `Usuario` guarda nome, senha, uid e gid; cada usuário pertence a um grupo. Só o root (uid 0) pode usar `useradd` e `groupadd`, que validam nome repetido, grupo inexistente e limite de espaço.

| Operação                                                    | Permissão exigida  |
| ------------------------------------------------------------- | ------------------- |
| `cat`, `cp` (origem)                                      | `r` no arquivo    |
| `echo >`, `touch` (existente), `cp` (destino existente) | `w` no arquivo    |
| `exec`                                                      | `x` no arquivo    |
| criar, `rm`, `mv`, `mkdir`                              | `w` no diretório |
| `ls`                                                        | `r` no diretório |
| `cd`                                                        | `x` no diretório |

### Alocação de blocos

O disco é `char disco[64 * 16]`: 64 blocos de 16 bytes, com um bitmap marcando os blocos ocupados. A teoria apresenta três métodos:

- **Contígua:** o arquivo ocupa blocos seguidos; o acesso é rápido, mas sofre fragmentação externa.
- **Encadeada:** cada bloco aponta para o próximo; não fragmenta, mas o acesso direto é lento.
- **Indexada (a usada aqui):** o FCB tem um vetor `blocos[]` (o bloco de índice) com os números dos blocos do arquivo, em ordem.

Ao escrever, o conteúdo é dividido em pedaços de 16 bytes (`teto(tamanho/16)` blocos) e cada pedaço vai para o **primeiro bloco livre**. Por isso os blocos não precisam ser contíguos, e o acesso ao *i*-ésimo bloco é direto (`blocos[i]`). O limite por arquivo é de 8 entradas no índice (128 bytes). O comando `stat` mostra o índice e o conteúdo de cada bloco, e o `df` mostra o mapa do disco.

## Exemplos de uso e comparação com Linux

**Árvore, criação e permissões:**

```
alice@fs:/$ mkdir docs
alice@fs:/$ cd docs
alice@fs:/docs$ echo "Ola mundo, este texto ocupa varios blocos!" > nota.txt
alice@fs:/docs$ echo 12345 > dados.num
alice@fs:/docs$ ls
  4 -rw-r--r-- alice   alunos         5 18/11/2026 10:48 dados.num
  3 -rw-r--r-- alice   alunos        42 18/11/2026 10:48 nota.txt
alice@fs:/docs$ chmod 600 nota.txt
alice@fs:/docs$ su bob
bob@fs:/docs$ cat nota.txt
Permissao negada: 'nota.txt' (precisa de r)
bob@fs:/docs$ cat dados.num
12345
bob@fs:/docs$ echo x > dados.num
Permissao negada: 'dados.num' (precisa de w)
```

**Usuários, grupos e senha:**

```
alice@fs:/$ useradd michael alunos
Permissao negada: apenas o root pode usar 'useradd'
alice@fs:/$ su root
Senha: admin
root@fs:/$ groupadd professores
Grupo 'professores' criado (gid=300)
root@fs:/$ useradd michael professores 1234
Usuario 'michael' criado (uid=1003, grupo professores)
root@fs:/$ useradd diana alunos
Usuario 'diana' criado (uid=1004, grupo alunos)
root@fs:/$ groups
root         gid=0   : root
alunos       gid=100 : alice bob diana
visitantes   gid=200 : carlos
professores  gid=300 : michael
root@fs:/$ su diana
diana@fs:/$ su michael
Senha: errado
su: senha incorreta
```

**Blocos indexados não contíguos:** depois de criar `a.txt` (blocos 0 e 1) e `c.txt` (bloco 2) e apagar `a.txt`, um arquivo de 46 bytes ocupa os blocos livres 0, 1 e 3:

```
alice@fs:/$ stat d.txt
  Nome: d.txt
 Inode: 4   Tipo: caractere   Tamanho: 46 bytes
Acesso: (0644/-rw-r--r--)  Dono: alice (uid 1000)  Grupo: alunos (gid 100)
...
Blocos (indice): 3 -> 0 1 3
  bloco  0: "0123456789abcdef"
  bloco  1: "ghijklmnopqrstuv"
  bloco  3: "wxyz0123456789"
alice@fs:/$ df
Disco: 64 blocos de 16 bytes | livres: 60
  0: ####............
```

**Comparação com o Linux real:**

| Simulador          | Linux                | Diferença                                                           |
| ------------------ | -------------------- | -------------------------------------------------------------------- |
| `ls`             | `ls -li`           | mesmo formato (inode, permissões, dono, grupo, tamanho, data, nome) |
| `stat arq`       | `stat arq`         | também mostra o índice de blocos e o conteúdo de cada bloco       |
| `chmod 640 arq`  | `chmod 640 arq`    | só o modo octal numérico                                           |
| `echo txt > arq` | `echo txt > arq`   | o simulador não tem `>>` (anexar)                                 |
| `exec run.sh`    | `./run.sh`         | só mostra o conteúdo; exige `x` e tipo programa                  |
| `su bob`         | `su bob`           | pede senha só se o usuário tiver uma (root: `admin`)                |
| `useradd michael professores 1234` | `useradd -g professores michael` + `passwd` | senha opcional já no mesmo comando                |
| `groupadd professores` | `groupadd professores` | gid gerado automaticamente                                   |
| `groups`         | `cat /etc/group`   | lista cada grupo com seus membros                                    |
| `df`             | `df` / `debugfs` | mostra o mapa de blocos do disco simulado                            |
| `rm dir`         | `rmdir dir`        | remove só diretórios vazios                                        |

Os nomes são de um único nível (sem caminhos como `a/b`); para navegar usa-se `cd` com `..` e `/`.
