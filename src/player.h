#ifndef PLAYER_H
#define PLAYER_H

#include "board.h"

#define MAX_NOME_JOGADOR  32
#define MOEDAS_INICIAIS  150
#define BONUS_MARCO_ZERO 100
#define META_PONTOS       20   

typedef enum { TIPO_HUMANO, TIPO_BOT } TipoJogador;

typedef struct {
    char        nome[MAX_NOME_JOGADOR];
    int         moedas;
    int         pontos[4];        
    Casa       *posicao;
    int         turnos_bloqueado;
    TipoJogador tipo;
    int         avatar_id; 
    int         cartas_acao[2]; 
    int         turnos_festa;   
} Jogador;

Jogador jogador_criar(const char *nome, Casa *inicio, TipoJogador tipo, int avatar_id);
int     jogador_venceu(const Jogador *j);

#endif 
