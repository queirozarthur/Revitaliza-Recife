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
    TURNO_MOSTRANDO_CARTA,        /* carta puxada, aguardando fechar        */
    TURNO_MOSTRANDO_PROPRIEDADE   /* propriedade — comprar ou pagar aluguel */
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

    /* propriedade sob análise (válido em TURNO_MOSTRANDO_PROPRIEDADE) */
    Casa *casa_ativa;
    int   eh_aluguel;   /* 0 = compra livre, 1 = aluguel forçado */
} AnimacaoTurno;

/* ------------------------------------------------------------------ */
/* Paleta dos pinos (indexada pelo número do jogador 0-3)              */
/* ------------------------------------------------------------------ */
extern const Color COR_PINO[4];
extern Texture2D texturas_avatar[4];
extern Texture2D textura_tabuleiro;

/* ------------------------------------------------------------------ */
/* API de renderização                                                  */
/* ------------------------------------------------------------------ */
Rectangle render_casa_rect(int id);

void render_tabuleiro(const Tabuleiro *tab,
                      const Jogador *jogadores, int num_jogadores, int jogador_atual,
                      const AnimacaoTurno *anim);

void render_hud(const Jogador *jogadores, int num_jogadores, int jogador_atual, int jogador_focado,
                int ultimo_dado);

void render_dado_simples(int face, int cx, int cy, int raio, int jitter_x, int jitter_y, int destaque);

void render_dado(const AnimacaoTurno *anim);

void render_carta_overlay(const AnimacaoTurno *anim);

void render_propriedade_overlay(const AnimacaoTurno *anim, const Jogador *jogador);

#endif /* RENDER_H */
