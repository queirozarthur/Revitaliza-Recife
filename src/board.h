#ifndef BOARD_H
#define BOARD_H

#define MAX_NOME_CASA 64

/* Tipos de casa no tabuleiro */
typedef enum {
    CASA_INICIO,
    CASA_PROPRIEDADE,
    CASA_EVENTO,
    CASA_SORTE,
    CASA_AZAR
} TipoCasa;

/* Setor de impacto da propriedade */
typedef enum {
    SETOR_NEUTRO,
    SETOR_TECNOLOGIA,
    SETOR_TURISMO,
    SETOR_COMERCIO
} SetorCasa;

/* Nó da lista duplamente encadeada circular */
typedef struct Casa {
    int          id;
    char         nome[MAX_NOME_CASA];
    TipoCasa     tipo;
    SetorCasa    setor;
    int          custo;               /* custo para desenvolver */
    int          pontos;              /* pontos gerados ao setor */
    int          proprietario;        /* índice do jogador; -1 = livre */
    struct Casa *prev;
    struct Casa *next;
} Casa;

/* Contêiner do tabuleiro */
typedef struct {
    Casa *cabeca;   /* Marco Zero (id 0) */
    int   tamanho;
} Tabuleiro;

/* --- ciclo de vida --- */
Tabuleiro *tabuleiro_criar(void);
void       tabuleiro_destruir(Tabuleiro *tab);

/* --- navegação --- */
Casa *tabuleiro_mover(Casa *atual, int passos);
Casa *tabuleiro_buscar_id(const Tabuleiro *tab, int id);

/* --- debug --- */
void tabuleiro_imprimir(const Tabuleiro *tab);

#endif /* BOARD_H */
