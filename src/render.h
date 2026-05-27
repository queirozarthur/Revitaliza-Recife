#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "board.h"
#include "player.h"
#include "cards.h"

#define DURACAO_PASSO 0.45f

typedef enum {
    TURNO_AGUARDANDO,
    TURNO_DADO_GIRANDO,
    TURNO_PINO_MOVENDO,
<<<<<<< HEAD
    TURNO_ESPERANDO_COMPRA_CARTA, 
    TURNO_ANIMANDO_COMPRA_CARTA,  
    TURNO_MOSTRANDO_CARTA,        
    TURNO_MOSTRANDO_PROPRIEDADE,  
    TURNO_USANDO_ACAO,            
    TURNO_VENDENDO_PROPRIEDADE    
=======
    TURNO_ESPERANDO_COMPRA_CARTA, /* Aguarda clique no deck correspondente  */
    TURNO_ANIMANDO_COMPRA_CARTA,  /* Carta subindo do deck para o centro    */
    TURNO_MOSTRANDO_CARTA,        /* carta puxada, aguardando fechar        */
    TURNO_MOSTRANDO_PROPRIEDADE,  /* propriedade — comprar ou pagar aluguel */
    TURNO_USANDO_ACAO,            /* janela de confirmação de uso de carta de ação */
    TURNO_VENDENDO_PROPRIEDADE,   /* jogador sem moedas precisa vender para pagar  */
    TURNO_MENSAGEM_VEZ            /* exibição da mensagem de quem é a vez          */
>>>>>>> ad25ce8e79a891aa81d2dc509751a3447e27b255
} EstadoTurno;

typedef struct {
    EstadoTurno   estado;
    
    float timer_mensagem_vez;

    int   resultado;
    int   face_atual;
    float timer_dado;
    float proximo_flash;
    int   jitter_x;
    int   jitter_y;

    int     passos_restantes;
    float   timer_passo;
    Vector2 pos_inicio;
    Vector2 pos_fim;

    int          carta_deck_idx;
    float        timer_carta;     
    const Carta *carta_ativa;

    int    acao_carta_id;   
    int    acao_slot;       
    int    acao_alvo_idx;   

    Casa *casa_ativa;
    int   eh_aluguel;   

    int   venda_divida;       
    int   venda_scroll;       
    int   venda_selecionada;  
} AnimacaoTurno;

extern const Color COR_PINO[4];
extern Texture2D texturas_avatar[4];

extern Texture2D textura_costas_cartas[3];

extern Texture2D textura_cartas_acao[4];

extern Texture2D textura_tabuleiro;

Rectangle render_casa_rect(int id);

void render_tabuleiro(const Tabuleiro *tab,
                      const Jogador *jogadores, int num_jogadores, int jogador_atual,
                      const AnimacaoTurno *anim);

Rectangle obter_rect_deck(int tipo_deck); 
Rectangle obter_rect_carta_acao(int slot, int hover);

void render_hud(const Jogador *jogadores, int num_jogadores, int jogador_atual, int jogador_focado,
                int ultimo_dado);

void render_dado_simples(int face, int cx, int cy, int raio, int jitter_x, int jitter_y, int destaque);

void render_dado(const AnimacaoTurno *anim);

void render_carta_overlay(const AnimacaoTurno *anim);

void render_propriedade_overlay(const AnimacaoTurno *anim, const Jogador *jogador);
void render_venda_overlay(const AnimacaoTurno *anim, const Jogador *jogador,
                          const Tabuleiro *tab, int jogador_idx);
void render_hud_cartas_acao(Jogador *j, AnimacaoTurno *anim);
void render_acao_overlay(Jogador *jogadores, int num_jogadores, int jogador_atual, AnimacaoTurno *anim, Font fonte);
void render_confetti(int active);

#endif 