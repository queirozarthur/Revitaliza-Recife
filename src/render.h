#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "board.h"
#include "player.h"

/* Duração de cada passo do pino (segundos) */
#define DURACAO_PASSO 0.28f

/* ------------------------------------------------------------------ */
/* Estado da animação de turno                                          */
/* ------------------------------------------------------------------ */
typedef enum {
    TURNO_AGUARDANDO,    /* esperando o jogador apertar ESPAÇO  */
    TURNO_DADO_GIRANDO,  /* dado girando na tela                */
    TURNO_PINO_MOVENDO   /* pino andando casa por casa          */
} EstadoTurno;

typedef struct {
    EstadoTurno estado;

    /* dado */
    int   resultado;        /* valor final sorteado (1–6)         */
    int   face_atual;       /* face exibida durante a animação    */
    float timer_dado;       /* tempo acumulado da animação        */
    float proximo_flash;    /* quando trocar a face               */
    int   jitter_x;         /* tremor horizontal do dado          */
    int   jitter_y;         /* tremor vertical   do dado          */

    /* pino */
    int     passos_restantes;
    float   timer_passo;    /* progresso do passo atual (0→DURACAO_PASSO) */
    Vector2 pos_inicio;     /* centro da casa de partida do passo */
    Vector2 pos_fim;        /* centro da casa de destino do passo */
} AnimacaoTurno;

/* ------------------------------------------------------------------ */
/* API de renderização                                                  */
/* ------------------------------------------------------------------ */

/* Retorna o retângulo de tela da casa com o dado id */
Rectangle render_casa_rect(int id);

/* Desenha o tabuleiro e o pino (com lerp durante animação) */
void render_tabuleiro(const Tabuleiro *tab, const Jogador *jogador,
                      const AnimacaoTurno *anim);

/* Painel lateral com dinheiro, pontos e último dado */
void render_hud(const Jogador *jogador, int ultimo_dado);

/* Overlay do dado (visível em DADO_GIRANDO e PINO_MOVENDO) */
void render_dado(const AnimacaoTurno *anim);

#endif /* RENDER_H */
