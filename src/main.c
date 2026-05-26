#include "raylib.h"
#include "board.h"
#include "cards.h"
#include "player.h"
#include "render.h"
#include <stdlib.h>

#define LARGURA  1280
#define ALTURA    720

typedef enum { TELA_MENU, TELA_JOGO, TELA_RESULTADO } TelaJogo;

/* ------------------------------------------------------------------ */
/* Paleta do menu                                                       */
/* ------------------------------------------------------------------ */
#define COR_FUNDO      (Color){ 10,  20,  40, 255}
#define COR_TITULO     (Color){255, 140,  30, 255}
#define COR_SUBTITULO  (Color){180, 210, 255, 255}
#define COR_BOTAO      (Color){ 20,  50,  90, 255}
#define COR_BOTAO_HOV  (Color){255, 140,  30, 255}
#define COR_TEXTO_BTN  (Color){220, 235, 255, 255}
#define COR_LINHA      (Color){255, 140,  30, 180}

#define NUM_BOTOES 4
typedef struct { Rectangle rect; const char *texto; } Botao;

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static int passou_marco_zero(int id_antes, int id_depois)
{
    return id_depois < id_antes;
}

/* Configura o lerp para o próximo passo do pino */
static void setup_passo(AnimacaoTurno *anim, const Jogador *jogador)
{
    Rectangle r_cur  = render_casa_rect(jogador->posicao->id);
    Rectangle r_prox = render_casa_rect(jogador->posicao->next->id);
    anim->pos_inicio = (Vector2){r_cur.x  + r_cur.width /2.0f,
                                 r_cur.y  + r_cur.height/2.0f};
    anim->pos_fim    = (Vector2){r_prox.x + r_prox.width /2.0f,
                                 r_prox.y + r_prox.height/2.0f};
    anim->timer_passo = 0.0f;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    InitWindow(LARGURA, ALTURA, "Revitaliza Recife — Porto Digital");
    SetTargetFPS(60);

    Tabuleiro     *tabuleiro = tabuleiro_criar();
    SistemaCartas *cartas    = cartas_criar();
    Jogador        jogador   = jogador_criar("Jogador 1", tabuleiro->cabeca);

    AnimacaoTurno anim    = {0};
    anim.estado           = TURNO_AGUARDANDO;
    anim.face_atual       = 1;
    int ultimo_dado       = 0;

    Botao botoes[NUM_BOTOES] = {
        { (Rectangle){540, 260, 200, 50}, "[1] Iniciar Jogo" },
        { (Rectangle){540, 330, 200, 50}, "[2] Ranking"      },
        { (Rectangle){540, 400, 200, 50}, "[3] Instrucoes"   },
        { (Rectangle){540, 470, 200, 50}, "[0] Sair"         },
    };

    TelaJogo estado = TELA_MENU;

    while (!WindowShouldClose()) {

        Vector2 mouse = GetMousePosition();
        float   dt    = GetFrameTime();

        /* ---- UPDATE ---- */
        switch (estado) {

            case TELA_MENU:
                for (int i = 0; i < NUM_BOTOES; i++) {
                    if (CheckCollisionPointRec(mouse, botoes[i].rect) &&
                        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (i == 0) estado = TELA_JOGO;
                        if (i == NUM_BOTOES - 1) goto sair;
                    }
                }
                break;

            case TELA_JOGO:

                /* --- ESPAÇO: inicia a jogada --- */
                if (anim.estado == TURNO_AGUARDANDO && IsKeyPressed(KEY_SPACE)) {
                    if (jogador.turnos_bloqueado > 0) {
                        jogador.turnos_bloqueado--;
                    } else {
                        anim.resultado        = rand() % 6 + 1;
                        anim.face_atual       = rand() % 6 + 1;
                        anim.timer_dado       = 0.0f;
                        anim.proximo_flash    = 0.0f;
                        anim.passos_restantes = anim.resultado;
                        anim.jitter_x         = 0;
                        anim.jitter_y         = 0;
                        anim.estado           = TURNO_DADO_GIRANDO;
                        ultimo_dado           = anim.resultado;
                    }
                }

                /* --- Animação do dado --- */
                if (anim.estado == TURNO_DADO_GIRANDO) {
                    anim.timer_dado    += dt;
                    anim.proximo_flash -= dt;

                    if (anim.proximo_flash <= 0.0f) {
                        anim.face_atual = rand() % 6 + 1;

                        if (anim.timer_dado < 0.8f) {
                            anim.proximo_flash = 0.06f;           /* rápido  */
                            anim.jitter_x = (rand() % 21) - 10;
                            anim.jitter_y = (rand() % 21) - 10;
                        } else if (anim.timer_dado < 1.4f) {
                            anim.proximo_flash = 0.13f;           /* médio   */
                            anim.jitter_x = (rand() % 13) - 6;
                            anim.jitter_y = (rand() % 13) - 6;
                        } else {
                            anim.proximo_flash = 0.30f;           /* lento   */
                            anim.jitter_x = (rand() % 5) - 2;
                            anim.jitter_y = (rand() % 5) - 2;
                        }
                    }

                    /* Dado para: mostra resultado e começa a mover o pino */
                    if (anim.timer_dado >= 1.8f) {
                        anim.face_atual = anim.resultado;
                        anim.jitter_x   = 0;
                        anim.jitter_y   = 0;
                        setup_passo(&anim, &jogador);
                        anim.estado = TURNO_PINO_MOVENDO;
                    }
                }

                /* --- Animação do pino (passo a passo) --- */
                if (anim.estado == TURNO_PINO_MOVENDO) {
                    anim.timer_passo += dt;

                    if (anim.timer_passo >= DURACAO_PASSO) {
                        int id_antes    = jogador.posicao->id;
                        jogador.posicao = jogador.posicao->next;
                        int id_depois   = jogador.posicao->id;

                        if (passou_marco_zero(id_antes, id_depois))
                            jogador.moedas += BONUS_MARCO_ZERO;

                        anim.passos_restantes--;

                        if (anim.passos_restantes <= 0) {
                            anim.estado = TURNO_AGUARDANDO;
                        } else {
                            setup_passo(&anim, &jogador);
                        }
                    }
                }
                break;

            case TELA_RESULTADO:
                if (IsKeyPressed(KEY_ENTER)) estado = TELA_MENU;
                break;
        }

        /* ---- DRAW ---- */
        BeginDrawing();
        ClearBackground(COR_FUNDO);

        switch (estado) {

            case TELA_MENU: {
                DrawLineEx((Vector2){100,130}, (Vector2){100,580}, 2, COR_LINHA);
                DrawLineEx((Vector2){1180,130},(Vector2){1180,580},2, COR_LINHA);

                int tw = MeasureText("REVITALIZA RECIFE", 50);
                DrawText("REVITALIZA RECIFE", LARGURA/2 - tw/2, 130, 50, COR_TITULO);
                tw = MeasureText("Revitalize a cidade. Conquiste o futuro.", 20);
                DrawText("Revitalize a cidade. Conquiste o futuro.",
                         LARGURA/2 - tw/2, 195, 20, COR_SUBTITULO);
                DrawLineEx((Vector2){340,230}, (Vector2){940,230}, 1, COR_LINHA);

                for (int i = 0; i < NUM_BOTOES; i++) {
                    int   hover     = CheckCollisionPointRec(mouse, botoes[i].rect);
                    Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;
                    Color cor_texto = hover ? COR_FUNDO     : COR_TEXTO_BTN;

                    DrawRectangleRec(botoes[i].rect, cor_fundo);
                    DrawRectangleLinesEx(botoes[i].rect, 1, COR_LINHA);

                    int bw = MeasureText(botoes[i].texto, 20);
                    DrawText(botoes[i].texto,
                             (int)(botoes[i].rect.x + botoes[i].rect.width /2 - bw/2),
                             (int)(botoes[i].rect.y + botoes[i].rect.height/2 - 10),
                             20, cor_texto);
                }

                tw = MeasureText("Cauã | Bernardo | João Arthur | Arthur Reis", 16);
                DrawText("Cauã | Bernardo | João Arthur | Arthur Reis",
                         LARGURA/2 - tw/2, 690, 16, COR_SUBTITULO);
                break;
            }

            case TELA_JOGO:
                render_tabuleiro(tabuleiro, &jogador, &anim);
                render_hud(&jogador, ultimo_dado);
                render_dado(&anim);          /* por cima de tudo */
                break;

            case TELA_RESULTADO:
                DrawText("FIM DE JOGO",
                         LARGURA/2 - 120, ALTURA/2 - 30, 40, COR_TITULO);
                DrawText("[ENTER] Voltar ao menu",
                         LARGURA/2 - 140, ALTURA/2 + 30, 20, COR_SUBTITULO);
                break;
        }

        EndDrawing();
    }

sair:
    cartas_destruir(cartas);
    tabuleiro_destruir(tabuleiro);
    CloseWindow();
    return 0;
}
