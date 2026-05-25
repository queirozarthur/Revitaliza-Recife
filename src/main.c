#include "raylib.h"
#include "board.h"
#include "cards.h"

#define COR_FUNDO      (Color){ 10,  20,  40, 255 }
#define COR_TITULO     (Color){ 255, 140,  30, 255 }
#define COR_SUBTITULO  (Color){ 180, 210, 255, 255 }
#define COR_BOTAO      (Color){ 20,  50,  90, 255 }
#define COR_BOTAO_HOV  (Color){ 255, 140,  30, 255 }
#define COR_TEXTO_BTN  (Color){ 220, 235, 255, 255 }
#define COR_LINHA      (Color){ 255, 140,  30, 180 }

#define LARGURA    800
#define ALTURA     600
#define NUM_BOTOES 4

typedef struct {
    Rectangle   rect;
    const char *texto;
} Botao;

int main(void)
{
    InitWindow(LARGURA, ALTURA, "Tabuleiro / Centro do Recife");
    SetTargetFPS(60);

    /* Cria o tabuleiro (lista circular) na inicialização */
    Tabuleiro    *tabuleiro = tabuleiro_criar();
    SistemaCartas *cartas   = cartas_criar();

    Botao botoes[NUM_BOTOES] = {
        { (Rectangle){ 300, 220, 200, 50 }, "[1] Iniciar Jogo" },
        { (Rectangle){ 300, 290, 200, 50 }, "[2] Ranking"      },
        { (Rectangle){ 300, 360, 200, 50 }, "[3] Instrucoes"   },
        { (Rectangle){ 300, 430, 200, 50 }, "[0] Sair"         },
    };

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        /* --- UPDATE --- */
        for (int i = 0; i < NUM_BOTOES; i++) {
            if (CheckCollisionPointRec(mouse, botoes[i].rect) &&
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (i == NUM_BOTOES - 1) {
                    cartas_destruir(cartas);
                    tabuleiro_destruir(tabuleiro);
                    CloseWindow();
                    return 0;
                }
                /* outros casos serão implementados nas próximas features */
            }
        }

        /* --- DRAW --- */
        BeginDrawing();
            ClearBackground(COR_FUNDO);

            DrawLineEx((Vector2){60,  100}, (Vector2){60,  500}, 2, COR_LINHA);
            DrawLineEx((Vector2){740, 100}, (Vector2){740, 500}, 2, COR_LINHA);

            DrawText("CENTRO DO RECIFE", 170, 100, 40, COR_TITULO);
            DrawText("Revitalize a cidade. Conquiste o futuro.", 155, 150, 18, COR_SUBTITULO);

            DrawLineEx((Vector2){200, 185}, (Vector2){600, 185}, 1, COR_LINHA);

            for (int i = 0; i < NUM_BOTOES; i++) {
                int   hover     = CheckCollisionPointRec(mouse, botoes[i].rect);
                Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;
                Color cor_texto = hover ? COR_FUNDO     : COR_TEXTO_BTN;

                DrawRectangleRec(botoes[i].rect, cor_fundo);
                DrawRectangleLinesEx(botoes[i].rect, 1, COR_LINHA);

                int tw = MeasureText(botoes[i].texto, 20);
                DrawText(botoes[i].texto,
                         (int)(botoes[i].rect.x + botoes[i].rect.width  / 2 - tw / 2),
                         (int)(botoes[i].rect.y + botoes[i].rect.height / 2 - 10),
                         20, cor_texto);
            }

            DrawText("Cauã | Bernardo | João Arthur | Arthur Reis",
                     185, 555, 16, COR_SUBTITULO);
        EndDrawing();
    }

    cartas_destruir(cartas);
    tabuleiro_destruir(tabuleiro);
    CloseWindow();
    return 0;
}
