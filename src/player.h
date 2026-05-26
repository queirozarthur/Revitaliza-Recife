#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"

#define MAX_NOME_JOGADOR  32
#define MOEDAS_INICIAIS  300
#define BONUS_MARCO_ZERO 100
#define META_PONTOS       20   /* pontos por setor para vencer */

typedef enum { TIPO_HUMANO, TIPO_BOT } TipoJogador;

typedef struct {
    char        nome[MAX_NOME_JOGADOR];
    int         moedas;
    int         pontos[4];        /* índice = SetorCasa (0=neutro ignorado) */
    Casa       *posicao;
    int         turnos_bloqueado;
    TipoJogador tipo;
} Jogador;

Jogador jogador_criar(const char *nome, Casa *inicio, TipoJogador tipo);
int     jogador_venceu(const Jogador *j);

#endif /* PLAYER_H */
