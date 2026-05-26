#include "raylib.h"
#include "board.h"
#include "cards.h"
#include "player.h"
#include "render.h"
#include "ranking.h"
#include <stdlib.h>
#include <stdio.h>

#define LARGURA       1280
#define ALTURA         720
#define NUM_JOGADORES    4

typedef enum { TELA_MENU, TELA_SELECAO, TELA_JOGO, TELA_RESULTADO } TelaJogo;

#define COR_FUNDO      (Color){ 10,  20,  40, 255}
#define COR_TITULO     (Color){255, 140,  30, 255}
#define COR_SUBTITULO  (Color){180, 210, 255, 255}
#define COR_BOTAO      (Color){ 20,  50,  90, 255}
#define COR_BOTAO_HOV  (Color){255, 140,  30, 255}
#define COR_TEXTO_BTN  (Color){220, 235, 255, 255}
#define COR_LINHA      (Color){255, 140,  30, 180}

/* Acesso ao jogador do turno atual */
#define J_ATUAL (&jogadores[jogador_atual])
#define IS_BOT  (jogadores[jogador_atual].tipo == TIPO_BOT)

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static int passou_marco_zero(int id_antes, int id_depois)
{
    return id_depois < id_antes;
}

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

static void iniciar_dado(AnimacaoTurno *anim, int *ultimo_dado)
{
    anim->resultado        = rand() % 6 + 1;
    anim->face_atual       = rand() % 6 + 1;
    anim->timer_dado       = 0.0f;
    anim->proximo_flash    = 0.0f;
    anim->passos_restantes = anim->resultado;
    anim->jitter_x         = 0;
    anim->jitter_y         = 0;
    anim->estado           = TURNO_DADO_GIRANDO;
    *ultimo_dado           = anim->resultado;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    InitWindow(LARGURA, ALTURA, "Revitaliza Recife — Porto Digital");
    SetTargetFPS(60);

    Tabuleiro     *tabuleiro     = NULL;
    SistemaCartas *cartas        = NULL;
    Jogador        jogadores[NUM_JOGADORES];
    int            jogador_atual   = 0;
    float          bot_delay       = -1.0f;  /* < 0 = timer não iniciado */
    int            turno_terminado = 0;

    AnimacaoTurno anim    = {0};
    anim.estado           = TURNO_AGUARDANDO;
    anim.face_atual       = 1;
    int ultimo_dado       = 0;

    /* Botões do menu principal */
#define NUM_BOTOES_MENU 4
    struct { Rectangle rect; const char *texto; } botoes_menu[NUM_BOTOES_MENU] = {
        { (Rectangle){540, 260, 200, 50}, "[1] Iniciar Jogo" },
        { (Rectangle){540, 330, 200, 50}, "[2] Ranking"      },
        { (Rectangle){540, 400, 200, 50}, "[3] Instrucoes"   },
        { (Rectangle){540, 470, 200, 50}, "[0] Sair"         },
    };

    /* Geometria dos botões de seleção (usada em update e draw) */
    const int SEL_BW = 110, SEL_BH = 80, SEL_GAP = 28;
    const int SEL_TOTAL_W = NUM_JOGADORES * SEL_BW + (NUM_JOGADORES - 1) * SEL_GAP;
    const int SEL_BX = LARGURA / 2 - SEL_TOTAL_W / 2;
    const int SEL_BY = 290;

    TelaJogo estado    = TELA_MENU;
    Ranking   ranking  = {0};   /* preenchido ao entrar em TELA_RESULTADO */

    while (!WindowShouldClose()) {

        Vector2 mouse = GetMousePosition();
        float   dt    = GetFrameTime();

        /* ==================================================================
         * UPDATE
         * ================================================================== */
        switch (estado) {

        /* ---- MENU ---- */
        case TELA_MENU:
            if (IsKeyPressed(KEY_ONE)  || IsKeyPressed(KEY_KP_1)) estado = TELA_SELECAO;
            if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) goto sair;
            for (int i = 0; i < NUM_BOTOES_MENU; i++) {
                if (CheckCollisionPointRec(mouse, botoes_menu[i].rect) &&
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (i == 0) estado = TELA_SELECAO;
                    if (i == NUM_BOTOES_MENU - 1) goto sair;
                }
            }
            break;

        /* ---- SELECAO ---- */
        case TELA_SELECAO: {
            int k_sel = 0;

            /* Teclado */
            for (int k = 1; k <= NUM_JOGADORES; k++) {
                if (IsKeyPressed(KEY_ONE   + (k - 1)) ||
                    IsKeyPressed(KEY_KP_1  + (k - 1)))
                    k_sel = k;
            }

            /* Mouse */
            for (int k = 1; k <= NUM_JOGADORES; k++) {
                Rectangle br = { SEL_BX + (k-1)*(SEL_BW+SEL_GAP), SEL_BY, SEL_BW, SEL_BH };
                if (CheckCollisionPointRec(mouse, br) &&
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    k_sel = k;
            }

            if (IsKeyPressed(KEY_ESCAPE)) { estado = TELA_MENU; break; }

            if (k_sel > 0) {
                /* Reinicia o jogo */
                if (tabuleiro) tabuleiro_destruir(tabuleiro);
                if (cartas)    cartas_destruir(cartas);
                tabuleiro = tabuleiro_criar();
                cartas    = cartas_criar();

                for (int i = 0; i < NUM_JOGADORES; i++) {
                    char nome[MAX_NOME_JOGADOR];
                    TipoJogador tipo;
                    if (i < k_sel) {
                        snprintf(nome, sizeof(nome), "Jogador %d", i + 1);
                        tipo = TIPO_HUMANO;
                    } else {
                        snprintf(nome, sizeof(nome), "Bot %c", 'A' + (i - k_sel));
                        tipo = TIPO_BOT;
                    }
                    jogadores[i] = jogador_criar(nome, tabuleiro->cabeca, tipo);
                }

                jogador_atual   = 0;
                bot_delay       = -1.0f;
                turno_terminado = 0;
                anim            = (AnimacaoTurno){0};
                anim.estado     = TURNO_AGUARDANDO;
                anim.face_atual = 1;
                ultimo_dado     = 0;
                estado          = TELA_JOGO;
            }
            break;
        }

        /* ---- JOGO ---- */
        case TELA_JOGO: {
            /* 1. Avança turno se o anterior terminou */
            if (turno_terminado) {
                if (jogador_venceu(J_ATUAL)) {
                    ranking         = ranking_criar(jogadores, NUM_JOGADORES);
                    ranking_ordenar(&ranking);
                    estado          = TELA_RESULTADO;
                    turno_terminado = 0;
                    break;
                }
                jogador_atual   = (jogador_atual + 1) % NUM_JOGADORES;
                bot_delay       = -1.0f;
                turno_terminado = 0;
            }

            /* 2. TURNO_AGUARDANDO */
            if (anim.estado == TURNO_AGUARDANDO) {
                if (IS_BOT) {
                    if (bot_delay < 0.0f) bot_delay = 1.0f;
                    bot_delay -= dt;
                    if (bot_delay <= 0.0f) {
                        bot_delay = -1.0f;
                        if (J_ATUAL->turnos_bloqueado > 0) {
                            J_ATUAL->turnos_bloqueado--;
                            turno_terminado = 1;
                        } else {
                            iniciar_dado(&anim, &ultimo_dado);
                        }
                    }
                } else {
                    if (IsKeyPressed(KEY_SPACE)) {
                        if (J_ATUAL->turnos_bloqueado > 0) {
                            J_ATUAL->turnos_bloqueado--;
                            turno_terminado = 1;
                        } else {
                            iniciar_dado(&anim, &ultimo_dado);
                        }
                    }
                }
            }

            /* 3. Animação do dado */
            if (anim.estado == TURNO_DADO_GIRANDO) {
                anim.timer_dado    += dt;
                anim.proximo_flash -= dt;

                if (anim.proximo_flash <= 0.0f) {
                    anim.face_atual = rand() % 6 + 1;
                    if (anim.timer_dado < 0.8f) {
                        anim.proximo_flash = 0.06f;
                        anim.jitter_x = (rand() % 21) - 10;
                        anim.jitter_y = (rand() % 21) - 10;
                    } else if (anim.timer_dado < 1.4f) {
                        anim.proximo_flash = 0.13f;
                        anim.jitter_x = (rand() % 13) - 6;
                        anim.jitter_y = (rand() % 13) - 6;
                    } else {
                        anim.proximo_flash = 0.30f;
                        anim.jitter_x = (rand() % 5) - 2;
                        anim.jitter_y = (rand() % 5) - 2;
                    }
                }

                if (anim.timer_dado >= 1.8f) {
                    anim.face_atual = anim.resultado;
                    anim.jitter_x   = 0;
                    anim.jitter_y   = 0;
                    setup_passo(&anim, J_ATUAL);
                    anim.estado = TURNO_PINO_MOVENDO;
                }
            }

            /* 4. Animação do pino */
            if (anim.estado == TURNO_PINO_MOVENDO) {
                anim.timer_passo += dt;

                if (anim.timer_passo >= DURACAO_PASSO) {
                    int id_antes     = J_ATUAL->posicao->id;
                    J_ATUAL->posicao = J_ATUAL->posicao->next;
                    int id_depois    = J_ATUAL->posicao->id;

                    if (passou_marco_zero(id_antes, id_depois))
                        J_ATUAL->moedas += BONUS_MARCO_ZERO;

                    anim.passos_restantes--;

                    if (anim.passos_restantes <= 0) {
                        const Carta *carta = NULL;
                        switch (J_ATUAL->posicao->tipo) {
                            case CASA_SORTE:  carta = cartas_puxar_sorte(cartas);  break;
                            case CASA_AZAR:   carta = cartas_puxar_azar(cartas);   break;
                            case CASA_EVENTO: carta = cartas_puxar_evento(cartas); break;
                            default: break;
                        }

                        if (carta) {
                            anim.carta_ativa = carta;
                            anim.estado      = TURNO_MOSTRANDO_CARTA;
                        } else if (J_ATUAL->posicao->tipo == CASA_PROPRIEDADE) {
                            anim.casa_ativa = J_ATUAL->posicao;
                            if (J_ATUAL->posicao->proprietario < 0) {
                                /* livre para comprar */
                                anim.eh_aluguel = 0;
                                anim.estado     = TURNO_MOSTRANDO_PROPRIEDADE;
                            } else if (J_ATUAL->posicao->proprietario != jogador_atual) {
                                /* dono é outro jogador */
                                anim.eh_aluguel = 1;
                                anim.estado     = TURNO_MOSTRANDO_PROPRIEDADE;
                            } else {
                                /* jogador atual já é dono */
                                anim.casa_ativa = NULL;
                                anim.estado     = TURNO_AGUARDANDO;
                                turno_terminado = 1;
                            }
                        } else {
                            anim.estado     = TURNO_AGUARDANDO;
                            turno_terminado = 1;
                        }
                    } else {
                        setup_passo(&anim, J_ATUAL);
                    }
                }
            }
            break;
        }

        case TELA_RESULTADO:
            if (IsKeyPressed(KEY_ENTER)) estado = TELA_MENU;
            break;

        } /* fim switch update */

        /* ==================================================================
         * OVERLAYS — tratados fora do switch principal
         * ================================================================== */

        /* Carta puxada */
        if (anim.estado == TURNO_MOSTRANDO_CARTA && anim.carta_ativa) {
            if (!IS_BOT) {
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                    carta_aplicar_efeito(anim.carta_ativa, jogadores,
                                         NUM_JOGADORES, jogador_atual, tabuleiro);
                    anim.carta_ativa = NULL;
                    anim.estado      = TURNO_AGUARDANDO;
                    turno_terminado  = 1;
                }
            } else {
                if (bot_delay < 0.0f) bot_delay = 0.8f;
                bot_delay -= dt;
                if (bot_delay <= 0.0f) {
                    bot_delay = -1.0f;
                    carta_aplicar_efeito(anim.carta_ativa, jogadores,
                                         NUM_JOGADORES, jogador_atual, tabuleiro);
                    anim.carta_ativa = NULL;
                    anim.estado      = TURNO_AGUARDANDO;
                    turno_terminado  = 1;
                }
            }
        }

        /* Propriedade */
        if (anim.estado == TURNO_MOSTRANDO_PROPRIEDADE && anim.casa_ativa) {
            if (!IS_BOT) {
                if (!anim.eh_aluguel) {
                    /* Compra */
                    if (IsKeyPressed(KEY_C) &&
                        J_ATUAL->moedas >= anim.casa_ativa->custo) {
                        J_ATUAL->moedas -= anim.casa_ativa->custo;
                        J_ATUAL->pontos[anim.casa_ativa->setor] += anim.casa_ativa->pontos;
                        anim.casa_ativa->proprietario = jogador_atual;
                        anim.casa_ativa = NULL;
                        anim.estado     = TURNO_AGUARDANDO;
                        turno_terminado = 1;
                    } else if (IsKeyPressed(KEY_X)) {
                        anim.casa_ativa = NULL;
                        anim.estado     = TURNO_AGUARDANDO;
                        turno_terminado = 1;
                    }
                } else {
                    /* Aluguel */
                    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                        int aluguel = anim.casa_ativa->custo / 2;
                        J_ATUAL->moedas -= aluguel;
                        if (J_ATUAL->moedas < 0) J_ATUAL->moedas = 0;
                        int dono = anim.casa_ativa->proprietario;
                        if (dono >= 0 && dono < NUM_JOGADORES)
                            jogadores[dono].moedas += aluguel;
                        anim.casa_ativa = NULL;
                        anim.estado     = TURNO_AGUARDANDO;
                        turno_terminado = 1;
                    }
                }
            } else {
                /* Bot decide automaticamente */
                if (bot_delay < 0.0f) bot_delay = 0.8f;
                bot_delay -= dt;
                if (bot_delay <= 0.0f) {
                    bot_delay = -1.0f;
                    if (!anim.eh_aluguel) {
                        /* Bot compra se tiver moedas */
                        if (J_ATUAL->moedas >= anim.casa_ativa->custo) {
                            J_ATUAL->moedas -= anim.casa_ativa->custo;
                            J_ATUAL->pontos[anim.casa_ativa->setor] += anim.casa_ativa->pontos;
                            anim.casa_ativa->proprietario = jogador_atual;
                        }
                    } else {
                        int aluguel = anim.casa_ativa->custo / 2;
                        J_ATUAL->moedas -= aluguel;
                        if (J_ATUAL->moedas < 0) J_ATUAL->moedas = 0;
                        int dono = anim.casa_ativa->proprietario;
                        if (dono >= 0 && dono < NUM_JOGADORES)
                            jogadores[dono].moedas += aluguel;
                    }
                    anim.casa_ativa = NULL;
                    anim.estado     = TURNO_AGUARDANDO;
                    turno_terminado = 1;
                }
            }
        }

        /* ==================================================================
         * DRAW
         * ================================================================== */
        BeginDrawing();
        ClearBackground(COR_FUNDO);

        switch (estado) {

        /* ---- MENU ---- */
        case TELA_MENU: {
            DrawLineEx((Vector2){100,130}, (Vector2){100,580}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,130},(Vector2){1180,580},2, COR_LINHA);

            int tw = MeasureText("REVITALIZA RECIFE", 50);
            DrawText("REVITALIZA RECIFE", LARGURA/2 - tw/2, 130, 50, COR_TITULO);
            tw = MeasureText("Revitalize a cidade. Conquiste o futuro.", 20);
            DrawText("Revitalize a cidade. Conquiste o futuro.",
                     LARGURA/2 - tw/2, 195, 20, COR_SUBTITULO);
            DrawLineEx((Vector2){340,230}, (Vector2){940,230}, 1, COR_LINHA);

            for (int i = 0; i < NUM_BOTOES_MENU; i++) {
                int   hover     = CheckCollisionPointRec(mouse, botoes_menu[i].rect);
                Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;
                Color cor_texto = hover ? COR_FUNDO     : COR_TEXTO_BTN;

                DrawRectangleRec(botoes_menu[i].rect, cor_fundo);
                DrawRectangleLinesEx(botoes_menu[i].rect, 1, COR_LINHA);

                int bw = MeasureText(botoes_menu[i].texto, 20);
                DrawText(botoes_menu[i].texto,
                         (int)(botoes_menu[i].rect.x + botoes_menu[i].rect.width /2 - bw/2),
                         (int)(botoes_menu[i].rect.y + botoes_menu[i].rect.height/2 - 10),
                         20, cor_texto);
            }

            tw = MeasureText("Caue | Bernardo | Joao Arthur | Arthur Reis", 16);
            DrawText("Caue | Bernardo | Joao Arthur | Arthur Reis",
                     LARGURA/2 - tw/2, 690, 16, COR_SUBTITULO);
            break;
        }

        /* ---- SELECAO ---- */
        case TELA_SELECAO: {
            DrawLineEx((Vector2){100,130}, (Vector2){100,580}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,130},(Vector2){1180,580},2, COR_LINHA);

            int tw = MeasureText("REVITALIZA RECIFE", 46);
            DrawText("REVITALIZA RECIFE", LARGURA/2 - tw/2, 130, 46, COR_TITULO);

            const char *sub = "Quantos jogadores humanos?";
            tw = MeasureText(sub, 22);
            DrawText(sub, LARGURA/2 - tw/2, 210, 22, COR_SUBTITULO);

            DrawLineEx((Vector2){340,248}, (Vector2){940,248}, 1, COR_LINHA);

            /* Botões [1]–[4] */
            for (int k = 1; k <= NUM_JOGADORES; k++) {
                Rectangle br = { SEL_BX + (k-1)*(SEL_BW+SEL_GAP), SEL_BY,
                                  SEL_BW, SEL_BH };
                int hover = CheckCollisionPointRec(mouse, br);
                Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;

                DrawRectangleRec(br, cor_fundo);
                DrawRectangleLinesEx(br, 2, COR_PINO[k-1]);

                char txt[4];
                snprintf(txt, sizeof(txt), "[%d]", k);
                tw = MeasureText(txt, 26);
                DrawText(txt,
                         (int)(br.x + SEL_BW/2 - tw/2),
                         (int)(br.y + SEL_BH/2 - 13),
                         26, hover ? COR_FUNDO : COR_TEXTO_BTN);
            }

            /* Preview de jogadores por número selecionado via hover */
            int hover_k = 0;
            for (int k = 1; k <= NUM_JOGADORES; k++) {
                Rectangle br = { SEL_BX + (k-1)*(SEL_BW+SEL_GAP), SEL_BY,
                                  SEL_BW, SEL_BH };
                if (CheckCollisionPointRec(mouse, br)) hover_k = k;
            }
            if (hover_k == 0) hover_k = 1; /* default preview */

            int py = SEL_BY + SEL_BH + 30;
            for (int i = 0; i < NUM_JOGADORES; i++) {
                char preview[48];
                Color cor_preview;
                if (i < hover_k) {
                    snprintf(preview, sizeof(preview), "Jogador %d  (Humano)", i + 1);
                    cor_preview = COR_PINO[i];
                } else {
                    snprintf(preview, sizeof(preview), "Bot %c       (IA)",
                             'A' + (i - hover_k));
                    cor_preview = (Color){120, 130, 150, 200};
                }
                DrawCircle(LARGURA/2 - 130, py + 8, 6, COR_PINO[i]);
                tw = MeasureText(preview, 14);
                DrawText(preview, LARGURA/2 - 120, py, 14, cor_preview);
                py += 22;
            }

            const char *hint = "[1]–[4] escolher   [ESC] Voltar";
            tw = MeasureText(hint, 14);
            DrawText(hint, LARGURA/2 - tw/2, 660, 14,
                     (Color){130, 150, 190, 200});
            break;
        }

        /* ---- JOGO ---- */
        case TELA_JOGO:
            render_tabuleiro(tabuleiro, jogadores, NUM_JOGADORES, jogador_atual, &anim);
            render_hud(jogadores, NUM_JOGADORES, jogador_atual, ultimo_dado);
            render_dado(&anim);
            render_carta_overlay(&anim);
            render_propriedade_overlay(&anim, J_ATUAL);
            break;

        /* ---- RESULTADO ---- */
        case TELA_RESULTADO: {
            DrawLineEx((Vector2){100,80},  (Vector2){100,650}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,80}, (Vector2){1180,650},2, COR_LINHA);

            int tw = MeasureText("RANKING FINAL", 44);
            DrawText("RANKING FINAL", LARGURA/2 - tw/2, 80, 44, COR_TITULO);
            DrawLineEx((Vector2){340,138}, (Vector2){940,138}, 1, COR_LINHA);

            static const Color medalha[] = {
                {255, 215, 0, 255},   /* ouro   */
                {192, 192, 192, 255}, /* prata  */
                {205, 127, 50, 255},  /* bronze */
                {180, 210, 255, 200}, /* 4o     */
            };

            for (int i = 0; i < ranking.n; i++) {
                int ry = 170 + i * 100;

                /* Painel do jogador */
                Color fundo = (i == 0)
                              ? (Color){40, 35, 10, 220}
                              : (Color){15, 25, 50, 180};
                DrawRectangleRounded(
                    (Rectangle){200, ry, 880, 80}, 0.12f, 6, fundo);
                DrawRectangleRoundedLines(
                    (Rectangle){200, ry, 880, 80}, 0.12f, 6, medalha[i]);

                /* Posição */
                char pos[4];
                snprintf(pos, sizeof(pos), "%d.", i + 1);
                DrawText(pos, 220, ry + 24, 28, medalha[i]);

                /* Bolinha colorida identificadora */
                /* Find which original player this is by name */
                for (int p = 0; p < NUM_JOGADORES; p++) {
                    if (jogadores[p].nome[0] == ranking.entradas[i].nome[0]) {
                        DrawCircle(280, ry + 40, 14, COR_PINO[p]);
                        break;
                    }
                }

                /* Nome */
                DrawText(ranking.entradas[i].nome, 306, ry + 20, 22, WHITE);

                /* Pontos e moedas */
                char info[64];
                snprintf(info, sizeof(info), "%d pontos totais   $ %d moedas",
                         ranking.entradas[i].pontos_total,
                         ranking.entradas[i].moedas);
                DrawText(info, 306, ry + 48, 15, medalha[i]);
            }

            /* Campeão destacado */
            if (ranking.n > 0) {
                char campeao[64];
                snprintf(campeao, sizeof(campeao),
                         "Vencedor: %s!", ranking.entradas[0].nome);
                tw = MeasureText(campeao, 20);
                DrawText(campeao, LARGURA/2 - tw/2, 580, 20,
                         (Color){255, 215, 0, 255});
            }

            tw = MeasureText("[ENTER] Voltar ao menu", 18);
            DrawText("[ENTER] Voltar ao menu",
                     LARGURA/2 - tw/2, 630, 18, COR_SUBTITULO);
            break;
        }

        } /* fim switch draw */

        EndDrawing();
    }

sair:
    if (tabuleiro) tabuleiro_destruir(tabuleiro);
    if (cartas)    cartas_destruir(cartas);
    CloseWindow();
    return 0;
}
