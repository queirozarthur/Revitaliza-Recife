# 🏙️ Revitaliza Recife

> Jogo de tabuleiro digital desenvolvido para a disciplina de **Algoritmos e Estruturas de Dados (AED)** — CESAR School

---

## 📋 Sobre o Projeto

**Revitaliza Recife** é um jogo de tabuleiro estilo Monopoly, ambientado na revitalização do **Recife Antigo** e do **Porto Digital**. Os jogadores competem para acumular pontos nos três setores da cidade — Tecnologia, Turismo e Comércio — comprando propriedades, sorteando cartas e usando cartas de ação estratégicas.

### 👥 Equipe

| Membro | Papel |
|---|---|
| Cauã Parente| Desenvolvimento |
| Bernardo Guimarães| Desenvolvimento |
| João Arthur Duarte| Desenvolvimento |
| Arthur Queiroz | Desenvolvimento |

---

## 🎯 Objetivo

Ser o primeiro jogador a acumular **20 pontos em cada um dos 3 setores**:

- 🔵 **Tecnologia** — Porto Digital e startups
- 🟢 **Turismo** — Recife Antigo e cultura
- 🟠 **Comércio** — Bairro do Recife e negócios

---

## 🗂️ Estrutura de Arquivos

```
Revitaliza-Recife/
├── src/
│   ├── main.c          # Loop principal, máquinas de estado (telas e turno)
│   ├── board.c/h       # Lista duplamente encadeada circular (tabuleiro)
│   ├── hashtable.c/h   # Tabela hash com encadeamento separado (cartas)
│   ├── cards.c/h       # Sistema de cartas (Sorte, Azar, Evento, Ação)
│   ├── player.c/h      # Estrutura e lógica do jogador
│   ├── render.c/h      # Toda a renderização com Raylib
│   ├── ranking.c/h     # Ranking final com Insertion Sort
│   └── cJSON.c/h       # Biblioteca para parsing JSON (tabuleiro.json)
├── assets/
│   ├── Tabuleiro.png
│   ├── Avatar_Frevo.png
│   ├── Avatar_Manguebeat.png
│   ├── Avatar_Monumento.png
│   ├── Avatar_Tecnologia.png
│   ├── Carta_Sorte_Verso.png
│   ├── Carta_Azar_Verso.png
│   ├── Carta_Verso_Evento.png
│   ├── Carta_Acao_1.png .. Carta_Acao_4.png
│   └── tabuleiro.json  # Definição das 24 casas
├── ranking.txt         # Histórico persistente de partidas (gerado em tempo de execução)
└── makefile
```

---

## 🏗️ Estruturas de Dados

### 1. Lista Duplamente Encadeada Circular — `Tabuleiro`

O tabuleiro é implementado como uma lista circular onde cada nó (`Casa`) aponta para o anterior e o próximo. Isso permite mover o pino em qualquer direção e percorrer o tabuleiro infinitamente sem condições especiais de borda.

```
board.h → struct Casa { ... Casa *prev; Casa *next; }
board.h → struct Tabuleiro { Casa *cabeca; int tamanho; }
```

| Função | Descrição |
|---|---|
| `tabuleiro_criar()` | Lê `tabuleiro.json` via cJSON e monta a lista circular com 24 casas |
| `tabuleiro_destruir()` | Percorre a lista e libera toda a memória |
| `tabuleiro_mover(atual, passos)` | Avança/recua N posições seguindo `->next` ou `->prev` |
| `tabuleiro_buscar_id(tab, id)` | Busca linear O(n) pelo ID da casa |

### 2. Tabela Hash com Encadeamento Separado — `SistemaCartas`

As cartas do jogo são armazenadas em uma tabela hash de capacidade **31** (número primo), indexadas pelo ID inteiro de cada carta. Colisões são resolvidas por listas encadeadas em cada bucket.

```
hashtable.h → struct HashTable { HtNo **buckets; int capacidade; int tamanho; }
cards.h     → struct SistemaCartas { HashTable *tabela; Baralho sorte/azar/evento; }
```

| Função | Descrição |
|---|---|
| `ht_criar(capacidade)` | Aloca a tabela com N buckets zerados |
| `ht_inserir(ht, chave, valor)` | Hash por divisão + inserção na frente do bucket |
| `ht_buscar(ht, chave)` | Busca em O(1) amortizado |
| `ht_destruir(ht, free_fn)` | Libera todos os nós e chama `free_fn` em cada valor |
| `cartas_criar()` | Inicializa as 3 pilhas (Sorte/Azar/Evento) e registra cada carta na hash |
| `cartas_puxar_sorte/azar/evento()` | Retira do topo do baralho com wrap circular |

### 3. Insertion Sort — `ranking_ordenar`

Utilizado em **dois lugares** no código:

1. **`ranking.c → ranking_ordenar()`** — ordena os jogadores ao fim da partida, decrescente por `pontos_total`; empate desfeito por `moedas`.
2. **`main.c → TELA_ORDEM`** — ordena os jogadores pela rolagem inicial de dado para definir a ordem de jogada.

```c
for (int i = 1; i < r->n; i++) {
    EntradaRanking chave = r->entradas[i];
    int j = i - 1;
    while (j >= 0 && r->entradas[j].pontos_total < chave.pontos_total) {
        r->entradas[j + 1] = r->entradas[j];
        j--;
    }
    r->entradas[j + 1] = chave;
}
```

---

## 🖥️ Telas do Jogo

| Tela | Descrição |
|---|---|
| **Menu Principal** | Iniciar, Ranking, Instruções, Sair |
| **Seleção** | Escolha de quantos humanos (1–4) e avatar de cada um |
| **Ordem** | Cada jogador rola o dado; Insertion Sort define quem começa |
| **Jogo** | Loop principal com animações de dado, pino, cartas e propriedades |
| **Resultado** | Pódio com medalhas dourado/prata/bronze e estatísticas finais |
| **Ranking** | Histórico de partidas lido do `ranking.txt`; [DEL] limpa o arquivo |
| **Instruções** | Painel de regras em dois painéis lado a lado |

---

## 🎲 Mecânicas de Jogo

### Turno
1. Jogador (ou bot) rola o dado (1–6)
2. Pino se move animado casa a casa
3. A casa de destino determina a ação:

| Tipo de Casa | Efeito |
|---|---|
| **Propriedade** | Comprar (C) ou passar (X); se de outro jogador, paga aluguel |
| **Sorte** | Clica no deck para puxar uma carta — efeitos positivos |
| **Azar** | Clica no deck para puxar uma carta — efeitos negativos |
| **Evento** | Clica no deck — afeta todos os jogadores |
| **Marco Zero** | Recebe **+100 moedas** ao passar |
| **Bloqueio** | Perde o próximo turno |

### Cartas de Ação

Cada jogador começa com 2 cartas de ação aleatórias e pode usá-las **antes de rolar o dado**:

| # | Carta | Efeito |
|---|---|---|
| 1 | **Ação de Turismo** | +1 pt Turismo; +1 extra por propriedade de Turismo possuída |
| 2 | **Recife em Festa** | Dobra todos os pontos ganhos pelos próximos 2 turnos |
| 3 | **Ação de Comércio** | +1 pt Comércio; +1 extra por propriedade de Comércio possuída |
| 4 | **Parceria Estratégica** | Escolhe um parceiro; ambos ganham +1 em tudo; +1 bônus se tiverem setores diferentes |

### Venda Forçada

Se um jogador não tiver moedas suficientes para pagar aluguel, entra em modo de **venda forçada**: pode vender propriedades (recebe metade do custo de compra) até ter saldo suficiente.

---

## 🤖 Inteligência Artificial (Bots)

O jogo suporta **0 a 3 bots** por partida. A IA é implementada diretamente em C com lógica baseada em regras:

- **Delay simulado** (`bot_delay`) para imitar tempo de "pensamento"
- **25% de chance** por turno de usar uma carta de ação se tiver
- Bot **compra propriedade** se tiver moedas suficientes
- Bot **paga aluguel** automaticamente (paga o que tem se estiver sem saldo)
- Bot **puxa cartas** automaticamente após 0.5s

---

## 🃏 Catálogo de Cartas

### Sorte
| ID | Carta | Efeito |
|---|---|---|
| 100 | Patrocínio Cultural | +3 pts Turismo |
| 101 | Hackathon no Porto | +3 pts Tecnologia |
| 102 | Feira de Negócios | +3 pts Comércio |
| 103 | Investidor Anjo | +80 moedas |
| 104 | Movimento Express | Avance 3 casas |

### Azar
| ID | Carta | Efeito |
|---|---|---|
| 200 | Obra na Via | Perde 1 turno |
| 201 | Embargo Fiscal | Paga 50 moedas |
| 202 | Ato Vandálico | -1 pt Turismo |
| 203 | Retorno ao Início | Volta ao Marco Zero |

### Evento (afeta todos)
| ID | Carta | Efeito |
|---|---|---|
| 300 | Festival do Manguebeat | Todos +1 pt Turismo |
| 301 | Summit de Inovação | Todos +1 pt Tecnologia |
| 302 | Crise Econômica | Todos pagam 40 moedas |
| 303 | Parceria Porto Digital | Donos de Tecnologia ganham +1 pt |

---

## ⌨️ Controles

| Tecla | Ação |
|---|---|
| `ESPAÇO` | Rolar dado / Puxar carta / Confirmar |
| `C` | Comprar propriedade |
| `X` | Passar (não comprar) |
| `ENTER` | Confirmar ação / Fechar overlay |
| `ESC` | Cancelar / Fechar inspeção |
| `1` – `4` | Inspecionar jogador no HUD |
| `DEL` | Limpar histórico de ranking |

---

## 🔨 Como Compilar e Executar

### Pré-requisitos

- [MSYS2](https://www.msys2.org/) com `ucrt64`
- Raylib instalado em `C:/msys64/ucrt64/`

```bash
# Instalar Raylib via MSYS2 (ucrt64)
pacman -S mingw-w64-ucrt-x86_64-raylib
```

### Compilar

```bash
# Via makefile (dentro da pasta Revitaliza-Recife/)
mingw32-make

# Ou diretamente:
C:/msys64/ucrt64/bin/gcc.exe -Wall -Wextra -std=c11 \
  -Isrc -IC:/msys64/ucrt64/include \
  src/main.c src/board.c src/hashtable.c src/cards.c \
  src/player.c src/render.c src/ranking.c src/cJSON.c \
  -o recife.exe \
  -LC:/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

### Executar

```bash
./recife.exe
```

> ⚠️ Execute **a partir da pasta `Revitaliza-Recife/`** para que os assets e o `tabuleiro.json` sejam encontrados corretamente.

---

## 📦 Dependências

| Biblioteca | Uso |
|---|---|
| [Raylib](https://www.raylib.com/) | Janela, input, renderização 2D, texturas, áudio |
| [cJSON](https://github.com/DaveGamble/cJSON) | Parsing do `assets/tabuleiro.json` |

---

## 📁 Formato do Ranking

O arquivo `ranking.txt` é gerado automaticamente ao fim de cada partida no formato:

```
Nome;pontos_totais;moedas;dd/mm/aaaa hh:mm
```

Exemplo:
```
Jogador 1;65;240;27/05/2026 14:32
Bot A;48;120;27/05/2026 14:32
Bot B;31;80;27/05/2026 14:32
Bot C;22;60;27/05/2026 14:32
```
