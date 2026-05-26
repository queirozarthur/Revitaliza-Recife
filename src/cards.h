#ifndef CARDS_H
#define CARDS_H

#include "hashtable.h"
#include "board.h"

#define MODO_APRESENTACAO 1

#define MAX_TITULO        64
#define MAX_DESCRICAO    128
#define MAX_CARTAS_TIPO    8

typedef enum { CARTA_SORTE, CARTA_AZAR, CARTA_EVENTO } TipoCarta;

typedef enum {
    EFEITO_GANHAR_PONTOS,
    EFEITO_PERDER_PONTOS,
    EFEITO_GANHAR_MOEDAS,
    EFEITO_PERDER_MOEDAS,
    EFEITO_AVANCAR,
    EFEITO_VOLTAR_INICIO,
    EFEITO_PERDER_TURNO,
    EFEITO_TODOS_GANHAM_PONTOS,
    EFEITO_TODOS_PERDEM_MOEDAS,
    EFEITO_PROPRIETARIOS_BONUS
} TipoEfeito;

typedef struct {
    int        id;
    char       titulo[MAX_TITULO];
    char       descricao[MAX_DESCRICAO];
    TipoCarta  tipo;
    TipoEfeito efeito;
    SetorCasa  setor;
    int        valor;
} Carta;

typedef struct {
    int ids[MAX_CARTAS_TIPO];
    int topo;
    int total;
} Baralho;

typedef struct {
    HashTable *tabela;
    Baralho    sorte;
    Baralho    azar;
    Baralho    evento;
} SistemaCartas;

/* --- ciclo de vida --- */
SistemaCartas *cartas_criar(void);
void           cartas_destruir(SistemaCartas *sc);

/* --- operações de jogo --- */
const Carta *cartas_puxar_sorte(SistemaCartas *sc);
const Carta *cartas_puxar_azar(SistemaCartas *sc);
const Carta *cartas_puxar_evento(SistemaCartas *sc);
const Carta *cartas_buscar(const SistemaCartas *sc, int id);

/* --- efeito --- */
#include "player.h"
Casa *carta_aplicar_efeito(const Carta *carta,
                           Jogador *jogadores, int num_jogadores,
                           int idx_atual,
                           const Tabuleiro *tab);

void usar_carta_acao(Jogador *jogadores, int num_jogadores, int jogador_atual, int acao_id, int alvo_idx, Tabuleiro *tab);

#endif /* CARDS_H */