#include "ranking.h"
#include <string.h>

Ranking ranking_criar(const Jogador *jogadores, int num_jogadores)
{
    Ranking r = {0};
    r.n = num_jogadores;
    for (int i = 0; i < num_jogadores; i++) {
        strncpy(r.entradas[i].nome, jogadores[i].nome, MAX_NOME_JOGADOR - 1);
        r.entradas[i].pontos_total = jogadores[i].pontos[SETOR_TECNOLOGIA]
                                   + jogadores[i].pontos[SETOR_TURISMO]
                                   + jogadores[i].pontos[SETOR_COMERCIO];
        r.entradas[i].moedas = jogadores[i].moedas;
    }
    return r;
}

/* Insertion sort — decrescente por pontos_total; empate → mais moedas */
void ranking_ordenar(Ranking *r)
{
    for (int i = 1; i < r->n; i++) {
        EntradaRanking chave = r->entradas[i];
        int j = i - 1;
        while (j >= 0 &&
               (r->entradas[j].pontos_total < chave.pontos_total ||
               (r->entradas[j].pontos_total == chave.pontos_total &&
                r->entradas[j].moedas < chave.moedas))) {
            r->entradas[j + 1] = r->entradas[j];
            j--;
        }
        r->entradas[j + 1] = chave;
    }
}
