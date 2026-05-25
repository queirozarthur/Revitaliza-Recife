#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "board.h"
#include "player.h"

/* Retorna o retângulo de tela da casa com o dado id */
Rectangle render_casa_rect(int id);

/* Desenha todas as casas da lista circular + o peão do jogador */
void render_tabuleiro(const Tabuleiro *tab, const Jogador *jogador);

/* Painel lateral direito com dinheiro, pontos e último dado */
void render_hud(const Jogador *jogador, int ultimo_dado);

#endif /* RENDER_H */
