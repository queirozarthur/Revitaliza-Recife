#ifndef RANKING_H
#define RANKING_H

#include "player.h"

#define MAX_JOGADORES 4

typedef struct {
    char nome[MAX_NOME_JOGADOR];
    int  pontos_total;   /* soma dos 3 setores */
    int  moedas;
} EntradaRanking;

typedef struct {
    EntradaRanking entradas[MAX_JOGADORES];
    int            n;
} Ranking;

/* Constrói ranking a partir do array de jogadores */
Ranking ranking_criar(const Jogador *jogadores, int num_jogadores);

/* Ordena por pontos_total decrescente; empate → mais moedas (insertion sort) */
void ranking_ordenar(Ranking *r);

#endif /* RANKING_H */
