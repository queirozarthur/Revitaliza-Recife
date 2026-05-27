#ifndef BOARD_H
#define BOARD_H

#define MAX_NOME_CASA 64

typedef enum {
    CASA_INICIO,
    CASA_PROPRIEDADE,
    CASA_EVENTO,
    CASA_SORTE,
    CASA_AZAR
} TipoCasa;

typedef enum {
    SETOR_NEUTRO,
    SETOR_TECNOLOGIA,
    SETOR_TURISMO,
    SETOR_COMERCIO
} SetorCasa;

typedef struct Casa {
    int          id;
    char         nome[MAX_NOME_CASA];
    TipoCasa     tipo;
    SetorCasa    setor;
    int          custo;               
    int          pontos;              
    int          proprietario;        
    struct Casa *prev;
    struct Casa *next;
} Casa;

typedef struct {
    Casa *cabeca;   
    int   tamanho;
} Tabuleiro;

Tabuleiro *tabuleiro_criar(void);
void       tabuleiro_destruir(Tabuleiro *tab);

Casa *tabuleiro_mover(Casa *atual, int passos);
Casa *tabuleiro_buscar_id(const Tabuleiro *tab, int id);

void tabuleiro_imprimir(const Tabuleiro *tab);

#endif 
