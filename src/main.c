#include "raylib.h"
#include "board.h"
#include "cards.h"
#include "player.h"
#include "render.h"
#include <stdlib.h>

#define LARGURA  1280
#define ALTURA    720

/* ------------------------------------------------------------------ */
/* Estados de jogo                                                      */
/* ------------------------------------------------------------------ */
typedef enum {
    TELA_MENU,
    TELA_JOGO,
    TELA_RESULTADO
} TelaJogo;

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
/* Helpers de detecção de passagem pelo Marco Zero                      */
/* ------------------------------------------------------------------ */
static int passou_marco_zero(int id_antes, int id_depois)
{
    /* Wrap ocorreu sempre que a posição final é numericamente menor */
    return id_depois < id_antes;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    InitWindow(LARGURA, ALTURA, "Revitaliza Recife — Porto Digital");
    SetTargetFPS(60);

    /* --- Inicialização das estruturas de dados --- */
    Tabuleiro    *tabuleiro = tabuleiro_criar();
    SistemaCartas *cartas   = cartas_criar();
    Jogador        jogador  = jogador_criar("Jogador 1", tabuleiro->cabeca);

    int ultimo_dado = 0;

    /* Centralização dos botões do menu para 1280 x 720 */
    Botao botoes[NUM_BOTOES] = {
        { (Rectangle){540, 260, 200, 50}, "[1] Iniciar Jogo" },
        { (Rectangle){540, 330, 200, 50}, "[2] Ranking"      },
        { (Rectangle){540, 400, 200, 50}, "[3] Instrucoes"   },
        { (Rectangle){540, 470, 200, 50}, "[0] Sair"         },
    };

    TelaJogo estado = TELA_MENU;

    /* ---------------------------------------------------------------- */
    /* Game loop                                                         */
    /* ---------------------------------------------------------------- */
    while (!WindowShouldClose()) {

        Vector2 mouse = GetMousePosition();

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
                if (IsKeyPressed(KEY_SPACE)) {
                    if (jogador.turnos_bloqueado > 0) {
                        jogador.turnos_bloqueado--;
                        break;
                    }
                    int id_antes  = jogador.posicao->id;
                    ultimo_dado   = rand() % 6 + 1;
                    jogador.posicao = tabuleiro_mover(jogador.posicao, ultimo_dado);
                    int id_depois = jogador.posicao->id;
                    if (passou_marco_zero(id_antes, id_depois))
                        jogador.moedas += BONUS_MARCO_ZERO;
                    const Carta *carta = NULL;
                    switch (jogador.posicao->tipo) {
                        case CASA_SORTE:
                            carta = cartas_puxar_sorte(cartas);
                            break;
                        case CASA_AZAR:
                            carta = cartas_puxar_azar(cartas);
                            break;
                        case CASA_EVENTO:
                            carta = cartas_puxar_evento(cartas);
                            break;
                        default:
                            break;
                    }
                    if (carta)
                        carta_aplicar_efeito(carta, &jogador, 1, 0, tabuleiro);
                    if (jogador_venceu(&jogador))
                        estado = TELA_RESULTADO;
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
                /* linhas decorativas */
                DrawLineEx((Vector2){100, 130}, (Vector2){100, 580}, 2, COR_LINHA);
                DrawLineEx((Vector2){1180,130}, (Vector2){1180,580}, 2, COR_LINHA);

                /* título */
                int tw = MeasureText("REVITALIZA RECIFE", 50);
                DrawText("REVITALIZA RECIFE",
                         LARGURA/2 - tw/2, 130, 50, COR_TITULO);
                tw = MeasureText("Revitalize a cidade. Conquiste o futuro.", 20);
                DrawText("Revitalize a cidade. Conquiste o futuro.",
                         LARGURA/2 - tw/2, 195, 20, COR_SUBTITULO);

                DrawLineEx((Vector2){340, 230}, (Vector2){940, 230}, 1, COR_LINHA);

                /* botões */
                for (int i = 0; i < NUM_BOTOES; i++) {
                    int   hover     = CheckCollisionPointRec(mouse, botoes[i].rect);
                    Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;
                    Color cor_texto = hover ? COR_FUNDO     : COR_TEXTO_BTN;

                    DrawRectangleRec(botoes[i].rect, cor_fundo);
                    DrawRectangleLinesEx(botoes[i].rect, 1, COR_LINHA);

                    int bw = MeasureText(botoes[i].texto, 20);
                    DrawText(botoes[i].texto,
                             (int)(botoes[i].rect.x + botoes[i].rect.width  / 2 - bw/2),
                             (int)(botoes[i].rect.y + botoes[i].rect.height / 2 - 10),
                             20, cor_texto);
                }

                /* rodapé */
                tw = MeasureText("Cauã | Bernardo | João Arthur | Arthur Reis", 16);
                DrawText("Cauã | Bernardo | João Arthur | Arthur Reis",
                         LARGURA/2 - tw/2, 690, 16, COR_SUBTITULO);
                break;
            }

            case TELA_JOGO:
                render_tabuleiro(tabuleiro, &jogador);
                render_hud(&jogador, ultimo_dado);
                break;

            case TELA_RESULTADO:
                DrawText("FIM DE JOGO", LARGURA/2 - 120, ALTURA/2 - 30, 40, COR_TITULO);
                DrawText("[ENTER] Voltar ao menu", LARGURA/2 - 140, ALTURA/2 + 30, 20, COR_SUBTITULO);
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
