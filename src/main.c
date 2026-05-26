#include "raylib.h"
#include "board.h"
#include "cards.h"
#include "player.h"
#include "render.h"
#include <stdlib.h>
#include <stdio.h>

#define LARGURA  1280
#define ALTURA    720

typedef enum {
    TELA_MENU,
    TELA_JOGO,
    TELA_RESULTADO
} TelaJogo;

#define COR_FUNDO      (Color){ 10,  20,  40, 255}
#define COR_TITULO     (Color){255, 140,  30, 255}
#define COR_SUBTITULO  (Color){180, 210, 255, 255}
#define COR_BOTAO      (Color){ 20,  50,  90, 255}
#define COR_BOTAO_HOV  (Color){255, 140,  30, 255}
#define COR_TEXTO_BTN  (Color){220, 235, 255, 255}
#define COR_LINHA      (Color){255, 140,  30, 180}

#define NUM_BOTOES 4

typedef struct { Rectangle rect; const char *texto; } Botao;

static int passou_marco_zero(int id_antes, int id_depois)
{
    return id_depois < id_antes;
}

int main(void)
{
    InitWindow(LARGURA, ALTURA, "Revitaliza Recife — Porto Digital");
    SetTargetFPS(60);

    Tabuleiro     *tabuleiro = tabuleiro_criar();
    SistemaCartas *cartas    = cartas_criar();
    Jogador        jogador   = jogador_criar("Jogador 1", tabuleiro->cabeca);

    int ultimo_dado      = 0;
    int aguardando_compra = 0;   /* 1 = jogador pode comprar a casa atual */

    Botao botoes[NUM_BOTOES] = {
        { (Rectangle){540, 260, 200, 50}, "[1] Iniciar Jogo" },
        { (Rectangle){540, 330, 200, 50}, "[2] Ranking"      },
        { (Rectangle){540, 400, 200, 50}, "[3] Instrucoes"   },
        { (Rectangle){540, 470, 200, 50}, "[0] Sair"         },
    };

    TelaJogo estado = TELA_MENU;

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

                /* --- Resposta à oferta de compra --- */
                if (aguardando_compra) {
                    Casa *casa = jogador.posicao;
                    if (IsKeyPressed(KEY_C) && jogador.moedas >= casa->custo) {
                        /* Compra a propriedade */
                        casa->proprietario = 0;          /* índice do jogador */
                        jogador.moedas    -= casa->custo;
                        jogador.pontos[casa->setor] += casa->pontos;
                        aguardando_compra = 0;
                    } else if (IsKeyPressed(KEY_N)) {
                        /* Recusa a compra */
                        aguardando_compra = 0;
                    }
                    break;   /* bloqueia o dado enquanto decide */
                }
                /* --- Rolar dado --- */
                if (IsKeyPressed(KEY_SPACE)) {
                    if (jogador.turnos_bloqueado > 0) {
                        jogador.turnos_bloqueado--;
                        break;
                    }
                    int id_antes    = jogador.posicao->id;
                    ultimo_dado     = rand() % 6 + 1;
                    jogador.posicao = tabuleiro_mover(jogador.posicao, ultimo_dado);
                    int id_depois   = jogador.posicao->id;

                    if (passou_marco_zero(id_antes, id_depois))
                        jogador.moedas += BONUS_MARCO_ZERO;

                    /* --- Efeito da casa --- */
                    Casa        *casa  = jogador.posicao;
                    const Carta *carta = NULL;
                    switch (casa->tipo) {
                        case CASA_SORTE:
                            carta = cartas_puxar_sorte(cartas);
                            break;
                        case CASA_AZAR:
                            carta = cartas_puxar_azar(cartas);
                            break;
                        case CASA_EVENTO:
                            carta = cartas_puxar_evento(cartas);
                            break;
                        case CASA_PROPRIEDADE:
                            if (casa->proprietario == -1)
                                aguardando_compra = 1;   /* oferta de compra */
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
                DrawLineEx((Vector2){100, 130}, (Vector2){100, 580}, 2, COR_LINHA);
                DrawLineEx((Vector2){1180,130}, (Vector2){1180,580}, 2, COR_LINHA);

                int tw = MeasureText("REVITALIZA RECIFE", 50);
                DrawText("REVITALIZA RECIFE", LARGURA/2 - tw/2, 130, 50, COR_TITULO);
                tw = MeasureText("Revitalize a cidade. Conquiste o futuro.", 20);
                DrawText("Revitalize a cidade. Conquiste o futuro.",
                         LARGURA/2 - tw/2, 195, 20, COR_SUBTITULO);

                DrawLineEx((Vector2){340, 230}, (Vector2){940, 230}, 1, COR_LINHA);

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

                tw = MeasureText("Cauã | Bernardo | João Arthur | Arthur Reis", 16);
                DrawText("Cauã | Bernardo | João Arthur | Arthur Reis",
                         LARGURA/2 - tw/2, 690, 16, COR_SUBTITULO);
                break;
            }

            case TELA_JOGO:
                render_tabuleiro(tabuleiro, &jogador);
                render_hud(&jogador, ultimo_dado);

                /* Prompt de compra sobreposto */
                if (aguardando_compra) {
                    Casa *casa = jogador.posicao;
                    char  buf[128];

                    DrawRectangle(280, 300, 680, 120,
                                  (Color){10, 20, 40, 230});
                    DrawRectangleLines(280, 300, 680, 120,
                                       (Color){255, 140, 30, 255});

                    snprintf(buf, sizeof(buf),
                             "Comprar \"%s\" por %d moedas?", casa->nome, casa->custo);
                    int tw = MeasureText(buf, 18);
                    DrawText(buf, 620 - tw/2, 320, 18,
                             (Color){220, 235, 255, 255});

                    snprintf(buf, sizeof(buf),
                             "Seu saldo: %d moedas", jogador.moedas);
                    tw = MeasureText(buf, 15);
                    DrawText(buf, 620 - tw/2, 348, 15,
                             (Color){180, 210, 255, 200});

                    if (jogador.moedas >= casa->custo)
                        DrawText("[C] Comprar    [N] Recusar",
                                 620 - MeasureText("[C] Comprar    [N] Recusar", 16)/2,
                                 385, 16, (Color){255, 215, 0, 255});
                    else
                        DrawText("Saldo insuficiente.  [N] Continuar",
                                 620 - MeasureText("Saldo insuficiente.  [N] Continuar", 16)/2,
                                 385, 16, (Color){210, 50, 50, 255});
                }
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