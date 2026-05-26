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
    TURNO_ESPERANDO_COMPRA_CARTA, /* Aguarda clique no deck correspondente  */
    TURNO_ANIMANDO_COMPRA_CARTA,  /* Carta subindo do deck para o centro    */
    TURNO_MOSTRANDO_CARTA,        /* carta puxada, aguardando fechar        */
    TURNO_MOSTRANDO_PROPRIEDADE,  /* propriedade — comprar ou pagar aluguel */
    TURNO_USANDO_ACAO             /* janela de confirmação de uso de carta de ação */
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

    /* carta puxada (animação de voo e exibição) */
    int          carta_deck_idx;
    float        timer_carta;     /* de 0.0 até 1.0 durante a animação */
    const Carta *carta_ativa;

    /* Ação */
    int    acao_carta_id;   // 0 a 3 (ID da carta de ação selecionada)
    int    acao_slot;       // 0 ou 1 (Slot da mão do jogador)
    int    acao_alvo_idx;   // Alvo para carta de parceria

    /* propriedade sob análise (válido em TURNO_MOSTRANDO_PROPRIEDADE) */
    Casa *casa_ativa;
    int   eh_aluguel;   /* 0 = compra livre, 1 = aluguel forçado */
} AnimacaoTurno;

/* ------------------------------------------------------------------ */
/* Paleta dos pinos (indexada pelo número do jogador 0-3)              */
/* ------------------------------------------------------------------ */
extern const Color COR_PINO[4];
extern Texture2D texturas_avatar[4];

/* Texturas das Costas das Cartas (Azar, Sorte, Evento) */
extern Texture2D textura_costas_cartas[3];

/* Texturas das Cartas de Ação (1 a 4) */
extern Texture2D textura_cartas_acao[4];

extern Texture2D textura_tabuleiro;

/* ------------------------------------------------------------------ */
/* API de renderização                                                  */
/* ------------------------------------------------------------------ */
Rectangle render_casa_rect(int id);

void render_tabuleiro(const Tabuleiro *tab,
                      const Jogador *jogadores, int num_jogadores, int jogador_atual,
                      const AnimacaoTurno *anim);

Rectangle obter_rect_deck(int tipo_deck); /* 0: Sorte, 1: Azar, 2: Evento */
Rectangle obter_rect_carta_acao(int slot, int hover);

void render_hud(const Jogador *jogadores, int num_jogadores, int jogador_atual, int jogador_focado,
                int ultimo_dado);

void render_dado_simples(int face, int cx, int cy, int raio, int jitter_x, int jitter_y, int destaque);

void render_dado(const AnimacaoTurno *anim);

void render_carta_overlay(const AnimacaoTurno *anim);

void render_propriedade_overlay(const AnimacaoTurno *anim, const Jogador *jogador);
void render_hud_cartas_acao(Jogador *j, AnimacaoTurno *anim, Font fonte);
void render_acao_overlay(Jogador *jogadores, int num_jogadores, int jogador_atual, AnimacaoTurno *anim, Font fonte);

#endif /* RENDER_H */
