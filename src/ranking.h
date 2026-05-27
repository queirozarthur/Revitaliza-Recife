#ifndef RANKING_H
#define RANKING_H

#include "player.h"

#define MAX_JOGADORES 4

typedef struct {
    char nome[MAX_NOME_JOGADOR];
    int  pontos_total;   
    int  moedas;
} EntradaRanking;

typedef struct {
    EntradaRanking entradas[MAX_JOGADORES];
    int            n;
} Ranking;

Ranking ranking_criar(const Jogador *jogadores, int num_jogadores);

void ranking_ordenar(Ranking *r);

#endif 
