#include "raylib.h"
#include "board.h"
#include "cards.h"
#include "player.h"
#include "render.h"
#include "ranking.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define LARGURA       1280
#define ALTURA         720
#define NUM_JOGADORES    4

typedef enum { TELA_MENU, TELA_SELECAO, TELA_ORDEM, TELA_JOGO, TELA_RESULTADO, TELA_RANKING, TELA_INSTRUCOES } TelaJogo;

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
    anim->resultado        = GetRandomValue(1, 6);
    anim->face_atual       = GetRandomValue(1, 6);
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
    srand(time(NULL));
    InitWindow(LARGURA, ALTURA, "Revitaliza Recife — Porto Digital");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    
    /* Warm up PRNG to avoid repeating early values */
    for (int i = 0; i < 100; i++) {
        GetRandomValue(0, 100);
        rand();
    }

    Tabuleiro     *tabuleiro     = NULL;
    SistemaCartas *cartas        = NULL;
    Jogador        jogadores[NUM_JOGADORES];
    int            jogador_atual   = 0;
    int            hud_jogador_focado = -1;
    float          bot_delay       = -1.0f;  /* < 0 = timer não iniciado */
    int            turno_terminado = 0;

    AnimacaoTurno anim    = {0};
    anim.estado           = TURNO_AGUARDANDO;
    anim.face_atual       = 1;
    int ultimo_dado       = 0;

    texturas_avatar[0] = LoadTexture("assets/Avatar_Frevo.png");
    texturas_avatar[1] = LoadTexture("assets/Avatar_Manguebeat.png");
    texturas_avatar[2] = LoadTexture("assets/Avatar_Monumento.png");
    texturas_avatar[3] = LoadTexture("assets/Avatar_Tecnologia.png");
    textura_tabuleiro  = LoadTexture("assets/Tabuleiro.png");
    textura_costas_cartas[0] = LoadTexture("assets/Carta_Sorte_Verso.png");
    textura_costas_cartas[1] = LoadTexture("assets/Carta_Azar_Verso.png");
    textura_costas_cartas[2] = LoadTexture("assets/Carta_Verso_Evento.png");
    
    textura_cartas_acao[0] = LoadTexture("assets/Carta_Acao_1.png");
    textura_cartas_acao[1] = LoadTexture("assets/Carta_Acao_2.png");
    textura_cartas_acao[2] = LoadTexture("assets/Carta_Acao_3.png");
    textura_cartas_acao[3] = LoadTexture("assets/Carta_Acao_4.png");

    int num_humanos = 1;
    int selecao_fase = 0; // 0: num_humanos, 1: escolhendo avatares
    int humano_atual = 0;
    int avatar_escolhido[NUM_JOGADORES] = {-1, -1, -1, -1};
    int avatar_disponivel[4] = {1, 1, 1, 1};
    
    int ordem_rolagem_atual = 0;
    float ordem_timer_dado = 0.0f;
    int ordem_resultados[NUM_JOGADORES] = {0};
    int ordem_desempate[NUM_JOGADORES] = {0};
    int estado_desempate = 0;

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
            if (IsKeyPressed(KEY_TWO)  || IsKeyPressed(KEY_KP_2)) estado = TELA_RANKING;
            if (IsKeyPressed(KEY_THREE)|| IsKeyPressed(KEY_KP_3)) estado = TELA_INSTRUCOES;
            if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) goto sair;
            for (int i = 0; i < NUM_BOTOES_MENU; i++) {
                if (CheckCollisionPointRec(mouse, botoes_menu[i].rect) &&
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (i == 0) estado = TELA_SELECAO;
                    if (i == 1) estado = TELA_RANKING;
                    if (i == 2) estado = TELA_INSTRUCOES;
                    if (i == NUM_BOTOES_MENU - 1) goto sair;
                }
            }
            break;

        /* ---- SELECAO ---- */
        case TELA_SELECAO: {
            if (IsKeyPressed(KEY_ESCAPE)) { estado = TELA_MENU; break; }

            if (selecao_fase == 0) {
                /* Escolha a quantidade de humanos */
                int k_sel = 0;
                for (int k = 1; k <= NUM_JOGADORES; k++) {
                    if (IsKeyPressed(KEY_ONE + (k - 1)) || IsKeyPressed(KEY_KP_1 + (k - 1))) k_sel = k;
                    Rectangle br = { SEL_BX + (k-1)*(SEL_BW+SEL_GAP), SEL_BY, SEL_BW, SEL_BH };
                    if (CheckCollisionPointRec(mouse, br) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) k_sel = k;
                }

                if (k_sel > 0) {
                    num_humanos = k_sel;
                    selecao_fase = 1;
                    humano_atual = 0;
                    for(int i=0; i<4; i++) { avatar_escolhido[i] = -1; avatar_disponivel[i] = 1; }
                }
            } else if (selecao_fase == 1) {
                /* Escolha do avatar para cada humano */
                int av_sel = -1;
                for (int k = 0; k < 4; k++) {
                    if (!avatar_disponivel[k]) continue;
                    if (IsKeyPressed(KEY_ONE + k) || IsKeyPressed(KEY_KP_1 + k)) av_sel = k;
                    Rectangle br = { SEL_BX + k*(SEL_BW+SEL_GAP), SEL_BY + 100, SEL_BW, SEL_BH };
                    if (CheckCollisionPointRec(mouse, br) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) av_sel = k;
                }

                if (av_sel != -1) {
                    avatar_escolhido[humano_atual] = av_sel;
                    avatar_disponivel[av_sel] = 0;
                    humano_atual++;

                    if (humano_atual >= num_humanos) {
                        /* Preenche os bots com os avatares restantes */
                        for (int i = num_humanos; i < NUM_JOGADORES; i++) {
                            for(int a=0; a<4; a++) {
                                if(avatar_disponivel[a]) {
                                    avatar_escolhido[i] = a;
                                    avatar_disponivel[a] = 0;
                                    break;
                                }
                            }
                        }

                        /* Reinicia o jogo e cria jogadores */
                        if (tabuleiro) tabuleiro_destruir(tabuleiro);
                        if (cartas)    cartas_destruir(cartas);
                        tabuleiro = tabuleiro_criar();
                        cartas    = cartas_criar();

                        for (int i = 0; i < NUM_JOGADORES; i++) {
                            char nome[MAX_NOME_JOGADOR];
                            TipoJogador tipo;
                            if (i < num_humanos) {
                                snprintf(nome, sizeof(nome), "Jogador %d", i + 1);
                                tipo = TIPO_HUMANO;
                            } else {
                                snprintf(nome, sizeof(nome), "Bot %c", 'A' + (i - num_humanos));
                                tipo = TIPO_BOT;
                            }
                            jogadores[i] = jogador_criar(nome, tabuleiro->cabeca, tipo, avatar_escolhido[i]);
                        }

                        ordem_rolagem_atual = 0;
                        ordem_timer_dado = 0.0f;
                        estado_desempate = 0;
                        for (int i=0; i<NUM_JOGADORES; i++) {
                            ordem_desempate[i] = 1; // 1 means needs to roll
                            ordem_resultados[i] = 0;
                        }
                        anim = (AnimacaoTurno){0};
                        anim.estado = TURNO_DADO_GIRANDO;
                        estado = TELA_ORDEM;
                    }
                }
            }
            break;
        }

        /* ---- ORDEM ---- */
        case TELA_ORDEM: {
            if (ordem_rolagem_atual < NUM_JOGADORES) {
                if (ordem_desempate[ordem_rolagem_atual] == 0) {
                    ordem_rolagem_atual++;
                    break; // Skip this player this frame
                }

                ordem_timer_dado += dt;
                
                if (anim.estado == TURNO_AGUARDANDO) {
                    if (ordem_timer_dado > 1.5f) { // Pausa para ver o dado antes de ir pro próximo
                        ordem_rolagem_atual++;
                        ordem_timer_dado = 0.0f;
                        anim.estado = TURNO_DADO_GIRANDO;
                    }
                } else {
                    anim.estado = TURNO_DADO_GIRANDO;
                    anim.timer_dado = ordem_timer_dado;
                    anim.proximo_flash -= dt;

                    if (anim.proximo_flash <= 0.0f) {
                        anim.face_atual = GetRandomValue(1, 6);
                        anim.proximo_flash = 0.08f;
                        if (ordem_timer_dado < 1.5f) {
                            anim.jitter_x = GetRandomValue(-10, 10);
                            anim.jitter_y = GetRandomValue(-10, 10);
                        } else {
                            anim.jitter_x = GetRandomValue(-2, 2);
                            anim.jitter_y = GetRandomValue(-2, 2);
                        }
                    }
                    
                    if (ordem_timer_dado > 2.5f) { // Após 2.5s rolando
                        int val = GetRandomValue(1, 6);
                        ordem_resultados[ordem_rolagem_atual] += val;
                        
                        anim.face_atual = val;
                        anim.estado = TURNO_AGUARDANDO;
                        anim.jitter_x = 0;
                        anim.jitter_y = 0;
                        ordem_timer_dado = 0.0f;
                    }
                }
            } else {
                if (estado_desempate == 0 || estado_desempate == 1) {
                    int tem_empate = 0;
                    int proximo_desempate[NUM_JOGADORES] = {0};
                    
                    for (int i=0; i<NUM_JOGADORES; i++) {
                        if (ordem_desempate[i] == 0 && estado_desempate == 1) continue;
                        
                        int contagem = 0;
                        for (int j=0; j<NUM_JOGADORES; j++) {
                            if (ordem_resultados[i] == ordem_resultados[j]) contagem++;
                        }
                        if (contagem > 1) {
                            proximo_desempate[i] = 1;
                            tem_empate = 1;
                        }
                    }
                    
                    if (tem_empate) {
                        estado_desempate = 1;
                        ordem_rolagem_atual = 0;
                        for (int i=0; i<NUM_JOGADORES; i++) {
                            ordem_desempate[i] = proximo_desempate[i];
                        }
                        anim.estado = TURNO_DADO_GIRANDO;
                        ordem_timer_dado = 0.0f;
                    } else {
                        estado_desempate = 2; // Pronto pra ir
                        ordem_timer_dado = 0.0f;
                    }
                } else if (estado_desempate == 2) {
                    ordem_timer_dado += dt;
                    anim.estado = TURNO_AGUARDANDO;
                if (ordem_timer_dado > 3.0f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) { // Espera mostrar o resultado
                    /* Ordena os jogadores baseado no dado rolado (Insertion Sort para O(N^2) mas como N=4 é constante) */
                    for (int i = 1; i < NUM_JOGADORES; i++) {
                        Jogador key_j = jogadores[i];
                        int key_r = ordem_resultados[i];
                        int j = i - 1;

                        while (j >= 0 && ordem_resultados[j] < key_r) {
                            jogadores[j + 1] = jogadores[j];
                            ordem_resultados[j + 1] = ordem_resultados[j];
                            j = j - 1;
                        }
                        jogadores[j + 1] = key_j;
                        ordem_resultados[j + 1] = key_r;
                    }

                    jogador_atual   = 0;
                    hud_jogador_focado = -1;
                    turno_terminado = 0;
                    anim            = (AnimacaoTurno){0};
                    anim.estado     = TURNO_MENSAGEM_VEZ;
                    anim.timer_mensagem_vez = 2.0f;
                    anim.face_atual = 1;
                    ultimo_dado     = 0;
                    estado          = TELA_JOGO;
                }
            } // fecha else if (estado_desempate == 2)
            } // fecha else (de if ordem_rolagem_atual < NUM_JOGADORES)
            break;
        }

        /* ---- JOGO ---- */
        case TELA_JOGO: {
            /* Controle do HUD (mouse e teclado) */
            if (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) hud_jogador_focado = 0;
            if (IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) hud_jogador_focado = 1;
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) hud_jogador_focado = 2;
            if (IsKeyPressed(KEY_FOUR)  || IsKeyPressed(KEY_KP_4)) hud_jogador_focado = 3;
            if (IsKeyPressed(KEY_ESCAPE)) hud_jogador_focado = -1;
            
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                /* Verifica cliques na lista de jogadores do HUD */
                int hx = 962, hy = 352; /* Posição Y onde começa a mini-lista em render_hud */
                for (int i = 0; i < NUM_JOGADORES; i++) {
                    Rectangle br = { hx, hy - 2, 308 - 10, 20 };
                    if (CheckCollisionPointRec(mouse, br)) {
                        hud_jogador_focado = i;
                    }
                    hy += 20;
                }
            }

            /* 1. Avança turno se o anterior terminou */
            if (turno_terminado) {
                if (jogador_venceu(J_ATUAL)) {
                    ranking         = ranking_criar(jogadores, NUM_JOGADORES);
                    ranking_ordenar(&ranking);
                    estado          = TELA_RESULTADO;
                    turno_terminado = 0;

                    /* Salva o vencedor no arquivo de recordes */
                    {
                        time_t t = time(NULL);
                        struct tm *tm_info = localtime(&t);
                        char data_str[32];
                        strftime(data_str, sizeof(data_str), "%d/%m/%Y %H:%M", tm_info);
                        FILE *f = fopen("ranking.txt", "a");
                        if (f) {
                            fprintf(f, "%s;%d;%d;%s\n",
                                    ranking.entradas[0].nome,
                                    ranking.entradas[0].pontos_total,
                                    ranking.entradas[0].moedas,
                                    data_str);
                            fclose(f);
                        }
                    }
                    break;
                }
                jogador_atual   = (jogador_atual + 1) % NUM_JOGADORES;
                if (J_ATUAL->turnos_festa > 0) {
                    J_ATUAL->turnos_festa--;
                }
                bot_delay       = -1.0f;
                turno_terminado = 0;
                anim.estado = TURNO_MENSAGEM_VEZ;
                anim.timer_mensagem_vez = 2.0f;
            }
            
            /* 1.5 TURNO_MENSAGEM_VEZ */
            if (anim.estado == TURNO_MENSAGEM_VEZ) {
                anim.timer_mensagem_vez -= dt;
                if (anim.timer_mensagem_vez <= 0.0f) {
                    anim.estado = TURNO_AGUARDANDO;
                }
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
                            // Bot intelligence for Action Cards (25% chance to use a card if has one)
                            int has_card = (J_ATUAL->cartas_acao[0] != -1 || J_ATUAL->cartas_acao[1] != -1);
                            if (has_card && GetRandomValue(1, 100) <= 25) {
                                int slot = (J_ATUAL->cartas_acao[0] != -1) ? 0 : 1;
                                if (J_ATUAL->cartas_acao[0] != -1 && J_ATUAL->cartas_acao[1] != -1) {
                                    slot = GetRandomValue(0, 1);
                                }
                                anim.acao_slot = slot;
                                anim.acao_carta_id = J_ATUAL->cartas_acao[slot];
                                anim.acao_alvo_idx = (jogador_atual + 1) % NUM_JOGADORES; // Default target
                                anim.estado = TURNO_USANDO_ACAO;
                            } else {
                                iniciar_dado(&anim, &ultimo_dado);
                            }
                        }
                    }
                } else {
                    if (J_ATUAL->turnos_bloqueado > 0) {
                        if (IsKeyPressed(KEY_SPACE)) {
                            J_ATUAL->turnos_bloqueado--;
                            turno_terminado = 1;
                        }
                    } else {
                        // Check Action Card Clicks
                        Vector2 mouse = GetMousePosition();
                        int clicou_carta = 0;
                        for (int i=0; i<2; i++) {
                            if (J_ATUAL->cartas_acao[i] != -1) {
                                Rectangle rect = obter_rect_carta_acao(i, 0);
                                if (CheckCollisionPointRec(mouse, rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                    anim.acao_slot = i;
                                    anim.acao_carta_id = J_ATUAL->cartas_acao[i];
                                    anim.acao_alvo_idx = (jogador_atual + 1) % NUM_JOGADORES;
                                    anim.estado = TURNO_USANDO_ACAO;
                                    clicou_carta = 1;
                                }
                            }
                        }
                        
                        if (!clicou_carta && IsKeyPressed(KEY_SPACE)) {
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
                    anim.face_atual = GetRandomValue(1, 6);
                    if (anim.timer_dado < 1.2f) {
                        anim.proximo_flash = 0.06f;
                        anim.jitter_x = GetRandomValue(-10, 10);
                        anim.jitter_y = GetRandomValue(-10, 10);
                    } else if (anim.timer_dado < 2.2f) {
                        anim.proximo_flash = 0.13f;
                        anim.jitter_x = GetRandomValue(-6, 6);
                        anim.jitter_y = GetRandomValue(-6, 6);
                    } else {
                        anim.proximo_flash = 0.30f;
                        anim.jitter_x = GetRandomValue(-2, 2);
                        anim.jitter_y = GetRandomValue(-2, 2);
                    }
                }

                if (anim.timer_dado >= 2.8f) {
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
                            anim.estado      = TURNO_ESPERANDO_COMPRA_CARTA;
                            anim.timer_carta = 0.0f;
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

        case TELA_RANKING:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) estado = TELA_MENU;
            break;

        case TELA_INSTRUCOES:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) estado = TELA_MENU;
            break;

        } /* fim switch update */

        /* ==================================================================
         * OVERLAYS — tratados fora do switch principal
         * ================================================================== */
         
        /* Esperando jogador puxar a carta (clicando no deck ou apertando botão) */
        if (anim.estado == TURNO_ESPERANDO_COMPRA_CARTA && anim.carta_ativa) {
            int clicou = 0;
            if (!IS_BOT) {
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                    clicou = 1;
                } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int deck_ativo = -1;
                    if (anim.carta_ativa->tipo == CARTA_SORTE) deck_ativo = 0;
                    if (anim.carta_ativa->tipo == CARTA_AZAR)  deck_ativo = 1;
                    if (anim.carta_ativa->tipo == CARTA_EVENTO) deck_ativo = 2;
                    
                    if (deck_ativo != -1) {
                        Rectangle rect = obter_rect_deck(deck_ativo);
                        if (CheckCollisionPointRec(mouse, rect)) {
                            clicou = 1;
                        }
                    }
                }
            } else {
                /* Bot puxa a carta sozinho após um tempo */
                if (bot_delay < 0.0f) bot_delay = 0.5f;
                bot_delay -= dt;
                if (bot_delay <= 0.0f) {
                    bot_delay = -1.0f;
                    clicou = 1;
                }
            }

            if (clicou) {
                anim.estado = TURNO_ANIMANDO_COMPRA_CARTA;
                anim.timer_carta = 0.0f;
            }
        }
        
        /* Animação da carta subindo do deck */
        if (anim.estado == TURNO_ANIMANDO_COMPRA_CARTA) {
            anim.timer_carta += dt / 1.0f; /* 1.0 segundo de animação */
            if (anim.timer_carta >= 1.0f) {
                anim.timer_carta = 1.0f;
                anim.estado = TURNO_MOSTRANDO_CARTA;
            }
        }

        /* Mostrando a carta já puxada */
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
                        int multi = J_ATUAL->turnos_festa > 0 ? 2 : 1;
                        J_ATUAL->pontos[anim.casa_ativa->setor] += anim.casa_ativa->pontos * multi;
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
                    /* Aluguel — se não tiver moedas entra em venda forçada */
                    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                        int aluguel = anim.casa_ativa->custo / 2;
                        if (J_ATUAL->moedas < aluguel) {
                            /* Não tem saldo: abre overlay de venda */
                            anim.venda_divida      = aluguel;
                            anim.venda_selecionada = 0;
                            anim.estado            = TURNO_VENDENDO_PROPRIEDADE;
                        } else {
                            J_ATUAL->moedas -= aluguel;
                            int dono = anim.casa_ativa->proprietario;
                            if (dono >= 0 && dono < NUM_JOGADORES)
                                jogadores[dono].moedas += aluguel;
                            anim.casa_ativa = NULL;
                            anim.estado     = TURNO_AGUARDANDO;
                            turno_terminado = 1;
                        }
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
                            int multi = J_ATUAL->turnos_festa > 0 ? 2 : 1;
                            J_ATUAL->pontos[anim.casa_ativa->setor] += anim.casa_ativa->pontos * multi;
                            anim.casa_ativa->proprietario = jogador_atual;
                        }
                    } else {
                        int aluguel = anim.casa_ativa->custo / 2;
                        if (!IS_BOT && J_ATUAL->moedas < aluguel) {
                            /* Bot sem moedas: paga o que tem */
                        }
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
        
        /* Uso de Carta de Ação */
        if (anim.estado == TURNO_USANDO_ACAO) {
            if (!IS_BOT) {
                Vector2 mouse = GetMousePosition();
                bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                
                Rectangle rect_confirm = {710, 460, 180, 40};
                Rectangle rect_cancel = {910, 460, 180, 40};
                
                if (IsKeyPressed(KEY_ESCAPE) || (clicked && CheckCollisionPointRec(mouse, rect_cancel))) {
                    anim.estado = TURNO_AGUARDANDO;
                } else if (IsKeyPressed(KEY_ENTER) || (clicked && CheckCollisionPointRec(mouse, rect_confirm))) {
                    usar_carta_acao(jogadores, NUM_JOGADORES, jogador_atual, anim.acao_carta_id, anim.acao_alvo_idx, tabuleiro);
                    J_ATUAL->cartas_acao[anim.acao_slot] = -1;
                    anim.estado = TURNO_AGUARDANDO;
                } else if (anim.acao_carta_id == 3) {
                    int y_offset = 180;
                    for (int i = 0; i < NUM_JOGADORES; i++) {
                        if (i == jogador_atual) continue;
                        Rectangle btn = {710, 200 + y_offset, 380, 30};
                        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i) || (clicked && CheckCollisionPointRec(mouse, btn))) {
                            anim.acao_alvo_idx = i;
                        }
                        y_offset += 40;
                    }
                }
            } else {
                if (bot_delay < 0.0f) bot_delay = 1.0f;
                bot_delay -= dt;
                if (bot_delay <= 0.0f) {
                    bot_delay = -1.0f;
                    usar_carta_acao(jogadores, NUM_JOGADORES, jogador_atual, anim.acao_carta_id, anim.acao_alvo_idx, tabuleiro);
                    J_ATUAL->cartas_acao[anim.acao_slot] = -1;
                    anim.estado = TURNO_AGUARDANDO;
                }
            }
        }

        /* Venda forçada de propriedade */
        if (anim.estado == TURNO_VENDENDO_PROPRIEDADE) {
            /* Coleta propriedades do jogador atual */
            Casa *props[24];
            int   n_props = 0;
            Casa *c_iter = tabuleiro->cabeca;
            do {
                if (c_iter->tipo == CASA_PROPRIEDADE &&
                    c_iter->proprietario == jogador_atual)
                    props[n_props++] = c_iter;
                c_iter = c_iter->next;
            } while (c_iter != tabuleiro->cabeca);

            int falta = anim.venda_divida - J_ATUAL->moedas;

            /* Clique em uma linha da lista */
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                const int PW = 620, PH = 520;
                const int PX = (1280 - PW) / 2;
                const int PY = (720  - PH) / 2;
                int row_y = PY + 68 + 22 + 28 + 10 + 10 + 20; /* mesmo cálculo do render */
                for (int i = 0; i < n_props && row_y + 44 < PY + PH - 60; i++) {
                    Rectangle row = {(float)(PX+20), (float)row_y, (float)(PW-40), 38.0f};
                    if (CheckCollisionPointRec(mouse, row)) {
                        /* Vende: devolve metade do custo e remove propriedade */
                        int revenda = props[i]->custo / 2;
                        J_ATUAL->moedas += revenda;
                        /* Desconta os pontos que a propriedade gerou */
                        J_ATUAL->pontos[props[i]->setor] -= props[i]->pontos;
                        if (J_ATUAL->pontos[props[i]->setor] < 0)
                            J_ATUAL->pontos[props[i]->setor] = 0;
                        props[i]->proprietario = -1; /* libera a propriedade */
                        anim.venda_selecionada = 0;
                        break;
                    }
                    row_y += 44;
                }
            }

            /* Confirma pagamento quando tiver saldo suficiente */
            if ((IsKeyPressed(KEY_ENTER)) && falta <= 0) {
                int aluguel = anim.venda_divida;
                J_ATUAL->moedas -= aluguel;
                if (J_ATUAL->moedas < 0) J_ATUAL->moedas = 0;
                /* Paga ao dono se for aluguel */
                if (anim.casa_ativa && anim.eh_aluguel) {
                    int dono = anim.casa_ativa->proprietario;
                    if (dono >= 0 && dono < NUM_JOGADORES)
                        jogadores[dono].moedas += aluguel;
                }
                anim.casa_ativa = NULL;
                anim.estado     = TURNO_AGUARDANDO;
                turno_terminado = 1;
            }

            /* ESC: paga o que tem e continua (sem moedas negativas) */
            if (IsKeyPressed(KEY_ESCAPE)) {
                int pode_pagar = J_ATUAL->moedas;
                if (anim.casa_ativa && anim.eh_aluguel) {
                    int dono = anim.casa_ativa->proprietario;
                    if (dono >= 0 && dono < NUM_JOGADORES)
                        jogadores[dono].moedas += pode_pagar;
                }
                J_ATUAL->moedas = 0;
                anim.casa_ativa = NULL;
                anim.estado     = TURNO_AGUARDANDO;
                turno_terminado = 1;
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

            if (selecao_fase == 0) {
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
            } else if (selecao_fase == 1) {
                char sub[64];
                snprintf(sub, sizeof(sub), "Jogador %d, escolha seu avatar:", humano_atual + 1);
                tw = MeasureText(sub, 22);
                DrawText(sub, LARGURA/2 - tw/2, 210, 22, COR_SUBTITULO);

                DrawLineEx((Vector2){340,248}, (Vector2){940,248}, 1, COR_LINHA);

                const char* nomes_avatares[] = {"Frevo", "Manguebeat", "Monumento", "Tecnologia"};

                for (int k = 0; k < 4; k++) {
                    Rectangle br = { SEL_BX + k*(SEL_BW+SEL_GAP), SEL_BY + 100,
                                      SEL_BW, SEL_BH };
                    if (!avatar_disponivel[k]) {
                        DrawRectangleRec(br, (Color){30, 30, 30, 200});
                        if (texturas_avatar[k].id != 0) {
                            Rectangle src = {0, 0, (float)texturas_avatar[k].width, (float)texturas_avatar[k].height};
                            Rectangle dst = {br.x + br.width/2 - 25, br.y + 10, 50, 50};
                            DrawTexturePro(texturas_avatar[k], src, dst, (Vector2){0,0}, 0.0f, (Color){100, 100, 100, 255});
                        }
                        
                        int dono = -1;
                        for(int h=0; h < humano_atual; h++) {
                            if(avatar_escolhido[h] == k) { dono = h; break; }
                        }
                        if (dono != -1) {
                            char dono_txt[32];
                            snprintf(dono_txt, sizeof(dono_txt), "Jogador %d", dono + 1);
                            int dw = MeasureText(dono_txt, 14);
                            DrawText(dono_txt,
                                     (int)(br.x + SEL_BW/2 - dw/2),
                                     (int)(br.y + SEL_BH + 5),
                                     14, COR_PINO[dono]);
                        }
                        continue;
                    }

                    int hover = CheckCollisionPointRec(mouse, br);
                    Color cor_fundo = hover ? COR_BOTAO_HOV : COR_BOTAO;

                    DrawRectangleRec(br, cor_fundo);
                    DrawRectangleLinesEx(br, 2, COR_PINO[k]);

                    if (texturas_avatar[k].id != 0) {
                        Rectangle src = {0, 0, (float)texturas_avatar[k].width, (float)texturas_avatar[k].height};
                        Rectangle dst = {br.x + br.width/2 - 25, br.y + 10, 50, 50};
                        DrawTexturePro(texturas_avatar[k], src, dst, (Vector2){0,0}, 0.0f, WHITE);
                    }

                    tw = MeasureText(nomes_avatares[k], 14);
                    DrawText(nomes_avatares[k],
                             (int)(br.x + SEL_BW/2 - tw/2),
                             (int)(br.y + SEL_BH - 18),
                             14, hover ? COR_FUNDO : COR_TEXTO_BTN);
                }
            }

            const char *hint = "[1]–[4] escolher   [ESC] Voltar";
            tw = MeasureText(hint, 14);
            DrawText(hint, LARGURA/2 - tw/2, 660, 14,
                     (Color){130, 150, 190, 200});
            break;
        }

        /* ---- ORDEM ---- */
        case TELA_ORDEM: {
            DrawLineEx((Vector2){100,130}, (Vector2){100,580}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,130},(Vector2){1180,580},2, COR_LINHA);

            int tw = MeasureText("REVITALIZA RECIFE", 46);
            DrawText("REVITALIZA RECIFE", LARGURA/2 - tw/2, 130, 46, COR_TITULO);

            const char *sub = "Decidindo a ordem de jogada...";
            tw = MeasureText(sub, 22);
            DrawText(sub, LARGURA/2 - tw/2, 210, 22, COR_SUBTITULO);

            if (ordem_rolagem_atual < NUM_JOGADORES) {
                /* Exibe apenas o jogador atual rolando no centro da tela */
                int p = ordem_rolagem_atual;
                char texto[64];
                snprintf(texto, sizeof(texto), "%s", jogadores[p].nome);
                
                int tw = MeasureText(texto, 32);
                int start_x = LARGURA / 2 - tw / 2 - 40; // Desloca para a esquerda para acomodar o dado
                int py = 320;
                
                DrawText(texto, start_x, py, 32, COR_PINO[p]);
                DrawText("Rolando...", start_x, py + 40, 20, (Color){200, 200, 200, 255});
                
                /* Desenha o dado animado ao lado do nome */
                render_dado_simples(anim.face_atual, start_x + tw + 60, py + 25, 40, anim.jitter_x, anim.jitter_y, 1);
                
            } else {
                /* Exibe a lista final ordenada */
                int py = 250;
                const char *titulo = "Ordem Definida!";
                int tw = MeasureText(titulo, 32);
                DrawText(titulo, LARGURA/2 - tw/2, py, 32, COR_TITULO);
                py += 60;
                
                for (int i = 0; i < NUM_JOGADORES; i++) {
                    char texto[64];
                    snprintf(texto, sizeof(texto), "%d. %s (Tirou %d)", i + 1, jogadores[i].nome, ordem_resultados[i]);
                    
                    tw = MeasureText(texto, 24);
                    DrawText(texto, LARGURA/2 - tw/2, py, 24, COR_PINO[i]);
                    py += 40;
                }
                const char *hint = "Resultados ordenados! Pressione [ESPACO] para iniciar.";
                tw = MeasureText(hint, 18);
                DrawText(hint, LARGURA/2 - tw/2, py + 40, 18, COR_TITULO);
            }
            break;
        }

        /* ---- JOGO ---- */
        case TELA_JOGO: {
            int festa = 0;
            for (int i=0; i<NUM_JOGADORES; i++) {
                if (jogadores[i].turnos_festa > 0) { festa = 1; break; }
            }
            render_tabuleiro(tabuleiro, jogadores, NUM_JOGADORES, jogador_atual, &anim);
            render_confetti(festa);
            render_hud(jogadores, NUM_JOGADORES, jogador_atual, hud_jogador_focado, ultimo_dado);
            render_dado(&anim);
            render_carta_overlay(&anim);
            render_propriedade_overlay(&anim, J_ATUAL);
            render_hud_cartas_acao(J_ATUAL, &anim, GetFontDefault());
            render_acao_overlay(jogadores, NUM_JOGADORES, jogador_atual, &anim, GetFontDefault());
            render_venda_overlay(&anim, J_ATUAL, tabuleiro, jogador_atual);

            if (anim.estado == TURNO_MENSAGEM_VEZ) {
                DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 180});
                
                char msg[64];
                snprintf(msg, sizeof(msg), "Vez do %s", J_ATUAL->nome);
                int tw = MeasureText(msg, 60);
                int tx = LARGURA/2 - tw/2;
                int ty = 360 - 30;
                
                DrawRectangle(0, ty - 30, 1280, 120, (Color){20, 20, 30, 240});
                DrawRectangle(0, ty - 30, 1280, 5, COR_PINO[jogador_atual]);
                DrawRectangle(0, ty + 90, 1280, 5, COR_PINO[jogador_atual]);
                
                DrawText(msg, tx - 3, ty + 3, 60, BLACK);
                DrawText(msg, tx + 3, ty + 3, 60, BLACK);
                DrawText(msg, tx, ty, 60, COR_PINO[jogador_atual]);
            }
            break;
        }

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

        /* ---- RANKING (menu) ---- */
        case TELA_RANKING: {
            DrawLineEx((Vector2){100,80},  (Vector2){100,650}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,80}, (Vector2){1180,650},2, COR_LINHA);

            int tw = MeasureText("RANKING", 48);
            DrawText("RANKING", LARGURA/2 - tw/2, 80, 48, COR_TITULO);
            DrawLineEx((Vector2){340,142}, (Vector2){940,142}, 1, COR_LINHA);

            /* Tenta carregar arquivo de recordes */
            FILE *f = fopen("ranking.txt", "r");
            if (!f) {
                const char *vazio = "Nenhuma partida registrada ainda.";
                tw = MeasureText(vazio, 22);
                DrawText(vazio, LARGURA/2 - tw/2, 340, 22,
                         (Color){130, 150, 190, 200});
                const char *hint2 = "Termine uma partida para registrar seu recorde!";
                tw = MeasureText(hint2, 16);
                DrawText(hint2, LARGURA/2 - tw/2, 380, 16,
                         (Color){100, 120, 160, 180});
            } else {
                static const Color medalha_r[] = {
                    {255, 215,   0, 255},
                    {192, 192, 192, 255},
                    {205, 127,  50, 255},
                    {180, 210, 255, 200},
                };
                /* Cabeçalho da tabela */
                DrawText("#",      230, 158, 16, COR_SUBTITULO);
                DrawText("Nome",   290, 158, 16, COR_SUBTITULO);
                DrawText("Pontos", 700, 158, 16, COR_SUBTITULO);
                DrawText("Moedas", 870, 158, 16, COR_SUBTITULO);
                DrawText("Data",   990, 158, 16, COR_SUBTITULO);
                DrawLineEx((Vector2){210,180}, (Vector2){1070,180}, 1,
                           (Color){255,140,30,80});

                char linha[128];
                int pos = 0;
                int ry  = 192;
                while (fgets(linha, sizeof(linha), f) && pos < 10) {
                    char nome[MAX_NOME_JOGADOR];
                    int  pts = 0, moedas = 0;
                    char data[32] = "";
                    /* formato: "Nome;pontos;moedas;data\n" */
                    sscanf(linha, "%47[^;];%d;%d;%31[^\n]",
                           nome, &pts, &moedas, data);

                    Color c = (pos < 4) ? medalha_r[pos]
                                        : (Color){180, 210, 255, 200};

                    Color fundo_linha = (pos % 2 == 0)
                        ? (Color){15, 28, 55, 180}
                        : (Color){20, 38, 70, 160};

                    DrawRectangleRounded(
                        (Rectangle){210, ry, 860, 36}, 0.1f, 4, fundo_linha);
                    DrawRectangleRoundedLines(
                        (Rectangle){210, ry, 860, 36}, 0.1f, 4, (Color){255,140,30,40});

                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d.", pos + 1);
                    DrawText(buf,   230, ry + 9, 18, c);
                    DrawText(nome,  290, ry + 9, 18, WHITE);

                    char spts[16], smoe[16];
                    snprintf(spts, sizeof(spts), "%d", pts);
                    snprintf(smoe, sizeof(smoe), "%d", moedas);
                    DrawText(spts,  700, ry + 9, 18, c);
                    DrawText(smoe,  870, ry + 9, 18, c);
                    if (data[0]) DrawText(data, 990, ry + 9, 14,
                                         (Color){130,150,190,200});

                    ry  += 42;
                    pos++;
                }
                fclose(f);

                if (pos == 0) {
                    const char *vazio = "Arquivo encontrado, mas sem entradas.";
                    tw = MeasureText(vazio, 18);
                    DrawText(vazio, LARGURA/2 - tw/2, 340, 18,
                             (Color){130, 150, 190, 200});
                }
            }

            tw = MeasureText("[ESC] ou [ENTER] Voltar ao menu", 16);
            DrawText("[ESC] ou [ENTER] Voltar ao menu",
                     LARGURA/2 - tw/2, 640, 16, COR_SUBTITULO);
            break;
        }

        /* ---- INSTRUCOES ---- */
        case TELA_INSTRUCOES: {
            DrawLineEx((Vector2){100,80},  (Vector2){100,660}, 2, COR_LINHA);
            DrawLineEx((Vector2){1180,80}, (Vector2){1180,660},2, COR_LINHA);

            int tw = MeasureText("INSTRUCOES", 48);
            DrawText("INSTRUCOES", LARGURA/2 - tw/2, 80, 48, COR_TITULO);
            DrawLineEx((Vector2){340,142}, (Vector2){940,142}, 1, COR_LINHA);

            /* ---- Painel esquerdo ---- */
            DrawRectangleRounded((Rectangle){120,160, 480,440}, 0.06f, 6,
                                 (Color){15,25,50,200});
            DrawRectangleRoundedLines((Rectangle){120,160, 480,440}, 0.06f, 6,
                                      (Color){255,140,30,120});

            DrawText("OBJETIVO",      148, 178, 20, COR_TITULO);
            DrawText("Acumule pontos nos setores",  148, 208, 16, COR_SUBTITULO);
            DrawText("Tecnologia, Turismo e",       148, 228, 16, COR_SUBTITULO);
            DrawText("Comercio. O primeiro a",      148, 248, 16, COR_SUBTITULO);
            DrawText("atingir a meta vence!",        148, 268, 16, COR_SUBTITULO);

            DrawLineEx((Vector2){136,296}, (Vector2){584,296}, 1,
                       (Color){255,140,30,60});

            DrawText("TURNO",         148, 308, 20, COR_TITULO);
            DrawText("[ESPACO]  Rolar o dado",      148, 336, 16, COR_SUBTITULO);
            DrawText("Pino move automaticamente",   148, 356, 16, COR_SUBTITULO);
            DrawText("Clique no deck para",         148, 376, 16, COR_SUBTITULO);
            DrawText("puxar a carta sorteada",      148, 396, 16, COR_SUBTITULO);

            DrawLineEx((Vector2){136,424}, (Vector2){584,424}, 1,
                       (Color){255,140,30,60});

            DrawText("CASAS",         148, 436, 20, COR_TITULO);
            DrawText("Propriedade  ->  Comprar [C]",148, 464, 15, COR_SUBTITULO);
            DrawText("                 ou Passar [X]",148,484, 15, COR_SUBTITULO);
            DrawText("Sorte / Azar / Evento ->",    148, 504, 15, COR_SUBTITULO);
            DrawText("   Clique no deck para puxar",148, 524, 15, COR_SUBTITULO);
            DrawText("Marco Zero -> Bonus de moedas",148,548, 15, COR_SUBTITULO);
            DrawText("Bloqueio   -> Perde um turno", 148,568, 15, COR_SUBTITULO);

            /* ---- Painel direito ---- */
            DrawRectangleRounded((Rectangle){640,160, 520,440}, 0.06f, 6,
                                 (Color){15,25,50,200});
            DrawRectangleRoundedLines((Rectangle){640,160, 520,440}, 0.06f, 6,
                                      (Color){255,140,30,120});

            DrawText("CARTAS DE ACAO",668, 178, 20, COR_TITULO);
            DrawText("Voce pode ter ate 2 cartas.",  668, 208, 16, COR_SUBTITULO);
            DrawText("Clique na carta para usar",    668, 228, 16, COR_SUBTITULO);
            DrawText("antes de rolar o dado.",       668, 248, 16, COR_SUBTITULO);

            DrawLineEx((Vector2){656,278}, (Vector2){1144,278}, 1,
                       (Color){255,140,30,60});

            DrawText("Carta 1  Impulso Financeiro",  668, 292, 15, (Color){255,215,0,255});
            DrawText("  +20 moedas imediatamente",   668, 312, 14, COR_SUBTITULO);
            DrawText("Carta 2  Parceria Estrategica",668, 340, 15, (Color){100,200,255,255});
            DrawText("  Rouba moedas de outro jogador",668,360, 14, COR_SUBTITULO);
            DrawText("Carta 3  Inovacao",            668, 388, 15, (Color){100,255,180,255});
            DrawText("  Dobra pontos na proxima casa",668,408, 14, COR_SUBTITULO);
            DrawText("Carta 4  Greve Geral",         668, 436, 15, (Color){255,100,100,255});
            DrawText("  Bloqueia todos por 1 turno", 668, 456, 14, COR_SUBTITULO);

            DrawLineEx((Vector2){656,488}, (Vector2){1144,488}, 1,
                       (Color){255,140,30,60});

            DrawText("CONTROLES",    668, 500, 20, COR_TITULO);
            DrawText("[1]-[4]  Inspecionar jogador", 668, 528, 15, COR_SUBTITULO);
            DrawText("[ESC]    Fechar inspecao",      668, 548, 15, COR_SUBTITULO);
            DrawText("[ENTER]  Confirmar acoes",      668, 568, 15, COR_SUBTITULO);

            tw = MeasureText("[ESC] ou [ENTER] Voltar ao menu", 16);
            DrawText("[ESC] ou [ENTER] Voltar ao menu",
                     LARGURA/2 - tw/2, 642, 16, COR_SUBTITULO);
            break;
        }
        }

        EndDrawing();
    }

sair:
    for (int i = 0; i < 4; i++) {
        if (texturas_avatar[i].id != 0) UnloadTexture(texturas_avatar[i]);
        if (textura_cartas_acao[i].id != 0) UnloadTexture(textura_cartas_acao[i]);
    }
    for (int i = 0; i < 3; i++) {
        if (textura_costas_cartas[i].id != 0) UnloadTexture(textura_costas_cartas[i]);
    }
    if (textura_tabuleiro.id != 0) UnloadTexture(textura_tabuleiro);
    if (tabuleiro) tabuleiro_destruir(tabuleiro);
    if (cartas)    cartas_destruir(cartas);
    CloseWindow();
    return 0;
}