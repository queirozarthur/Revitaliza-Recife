#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "board.h"
#include "player.h"
#include "cards.h"

#define DURACAO_PASSO 0.28f

/* ------------------------------------------------------------------ */
/* Estado da animação de turno                                          */
/* ------------------------------------------------------------------ */
typedef enum {
    TURNO_AGUARDANDO,
    TURNO_DADO_GIRANDO,
    TURNO_PINO_MOVENDO,
    TURNO_MOSTRANDO_CARTA   /* carta puxada, aguardando o jogador fechar */
} EstadoTurno;

typedef struct {
    EstadoTurno   estado;

    /* dado */
    int   resultado;
    int   face_atual;
    float timer_dado;
    float proximo_flash;
    int   jitter_x;
    int   jitter_y;

    /* pino */
    int     passos_restantes;
    float   timer_passo;
    Vector2 pos_inicio;
    Vector2 pos_fim;

    /* carta puxada (válido em TURNO_MOSTRANDO_CARTA) */
    const Carta *carta_ativa;
} AnimacaoTurno;

/* ------------------------------------------------------------------ */
/* API de renderização                                                  */
/* ------------------------------------------------------------------ */
Rectangle render_casa_rect(int id);

void render_tabuleiro(const Tabuleiro *tab, const Jogador *jogador,
                      const AnimacaoTurno *anim);

void render_hud(const Jogador *jogador, int ultimo_dado);

void render_dado(const AnimacaoTurno *anim);

void render_carta_overlay(const AnimacaoTurno *anim);

#endif /* RENDER_H */
