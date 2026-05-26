#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Geometria do tabuleiro                                              */
/* ------------------------------------------------------------------ */
#define BX  20
#define BY  20
#define CS 110
#define SW 140
#define SH  88

/* Paleta de setores */
#define COR_TEC   (Color){ 30, 144, 255, 220}
#define COR_TUR   (Color){ 60, 179, 113, 220}
#define COR_COM   (Color){255, 165,   0, 220}
#define COR_INI   (Color){255, 215,   0, 220}
#define COR_SOR   (Color){ 50, 205,  50, 220}
#define COR_AZA   (Color){210,  50,  50, 220}
#define COR_EVT   (Color){160, 100, 220, 220}
#define COR_BORDA (Color){ 70,  90, 130, 255}
#define COR_PROP  (Color){ 30,  80, 130, 255}

/* Paleta dos pinos por jogador */
const Color COR_PINO[4] = {
    {240, 240, 255, 255},  /* J0: branco-azulado */
    {255, 230,  50, 255},  /* J1: amarelo         */
    {255, 100, 100, 255},  /* J2: rosa/vermelho   */
    {100, 220, 150, 255},  /* J3: verde claro     */
};

Texture2D texturas_avatar[4] = {0};
Texture2D textura_tabuleiro = {0};
Texture2D textura_costas_cartas[3] = {0};
Texture2D textura_cartas_acao[4] = {0};

/* Offsets por jogador para evitar sobreposição na mesma casa */
static const float PINO_OX[4] = {-7.0f,  7.0f, -7.0f,  7.0f};
static const float PINO_OY[4] = {-7.0f, -7.0f,  7.0f,  7.0f};

/* Dimensões do tabuleiro com a imagem carregada */
#define IMG_X 10.0f
#define IMG_Y 104.0f
#define IMG_W 938.0f
#define IMG_H 511.0f

/* ------------------------------------------------------------------ */
/* Posicionamento das casas                                            */
/* ------------------------------------------------------------------ */
Rectangle render_casa_rect(int id)
{
    if (textura_tabuleiro.id != 0) {
        float b_x = 8.0f; /* margem visual da borda marrom na imagem */
        float b_y = 8.0f;
        float cw = (IMG_W - 2.0f * b_x) / 7.0f;
        float ch = (IMG_H - 2.0f * b_y) / 7.0f;
        int gx = 0, gy = 0;
        
        if (id >= 0 && id <= 6) {
            gx = id; gy = 6;
        } else if (id >= 7 && id <= 12) {
            gx = 6; gy = 6 - (id - 6);
        } else if (id >= 13 && id <= 18) {
            gx = 6 - (id - 12); gy = 0;
        } else if (id >= 19 && id <= 23) {
            gx = 0; gy = id - 18;
        }
        
        return (Rectangle){IMG_X + b_x + gx * cw, IMG_Y + b_y + gy * ch, cw, ch};
    }

    if (id ==  0) return (Rectangle){BX,           BY + CS + 5*SH, CS, CS};
    if (id ==  6) return (Rectangle){BX + CS+5*SW, BY + CS + 5*SH, CS, CS};
    if (id == 12) return (Rectangle){BX + CS+5*SW, BY,             CS, CS};
    if (id == 18) return (Rectangle){BX,           BY,             CS, CS};

    if (id >= 1  && id <= 5)
        return (Rectangle){BX + CS + (id-1)*SW,   BY + CS + 5*SH, SW, CS};
    if (id >= 7  && id <= 11)
        return (Rectangle){BX + CS + 5*SW, BY + CS + (11-id)*SH,  CS, SH};
    if (id >= 13 && id <= 17)
        return (Rectangle){BX + CS + (17-id)*SW,  BY,             SW, CS};
    if (id >= 19 && id <= 23)
        return (Rectangle){BX, BY + CS + (id-19)*SH,              CS, SH};

    return (Rectangle){0, 0, 0, 0};
}

/* ------------------------------------------------------------------ */
/* Helpers internos — tabuleiro                                        */
/* ------------------------------------------------------------------ */

static Color cor_da_casa(const Casa *c)
{
    if (c->tipo == CASA_INICIO) return COR_INI;
    if (c->tipo == CASA_SORTE)  return COR_SOR;
    if (c->tipo == CASA_AZAR)   return COR_AZA;
    if (c->tipo == CASA_EVENTO) return COR_EVT;
    switch (c->setor) {
        case SETOR_TECNOLOGIA: return COR_TEC;
        case SETOR_TURISMO:    return COR_TUR;
        case SETOR_COMERCIO:   return COR_COM;
        default:               return COR_PROP;
    }
}

static void desenhar_nome(const char *nome, Rectangle r, int font)
{
    int max_chars = (int)(r.width - 6) / (font * 6 / 10 + 1);
    if (max_chars < 1) max_chars = 1;

    int len = (int)strlen(nome);
    if (len <= max_chars) {
        int tw = MeasureText(nome, font);
        DrawText(nome,
                 (int)(r.x + r.width/2  - tw/2),
                 (int)(r.y + r.height/2 - font/2 + 6),
                 font, WHITE);
        return;
    }

    int split = max_chars;
    while (split > 0 && nome[split] != ' ') split--;
    if (split == 0) split = max_chars;

    char linha1[MAX_NOME_CASA] = {0};
    strncpy(linha1, nome, (size_t)split);
    int tw1 = MeasureText(linha1,        font);
    int tw2 = MeasureText(nome+split+1,  font);
    int cy  = (int)(r.y + r.height/2) - font;

    DrawText(linha1,       (int)(r.x + r.width/2 - tw1/2), cy,          font, WHITE);
    DrawText(nome+split+1, (int)(r.x + r.width/2 - tw2/2), cy + font+2, font, WHITE);
}

/* Desenha um pino de jogador na posição (px, py) */
static void desenhar_pino(float px, float py, int idx_jogador,
                           const Jogador *j, int raio, int destaque)
{
    Color cor = COR_PINO[idx_jogador];
    int avatar_id = j->avatar_id;

    /* Sombra */
    DrawCircle((int)px + 2, (int)py + 2, raio, (Color){0, 0, 0, 120});

    if (avatar_id >= 0 && avatar_id < 4 && texturas_avatar[avatar_id].id != 0) {
        Rectangle src = {0, 0, (float)texturas_avatar[avatar_id].width, (float)texturas_avatar[avatar_id].height};
        Rectangle dst = {px, py, raio*2.0f, raio*2.0f};
        Vector2 origin = {raio, raio};
        
        /* Fundo branco para a textura e borda de destaque */
        DrawCircle((int)px, (int)py, raio, WHITE);
        DrawTexturePro(texturas_avatar[avatar_id], src, dst, origin, 0.0f, WHITE);
        
        /* Borda circular */
        DrawCircleLines((int)px, (int)py, raio,
                        destaque ? (Color){255, 255, 255, 220} : cor);
        if(destaque) {
            DrawCircleLines((int)px, (int)py, raio+1, cor);
        }
    } else {
        /* Corpo genérico fallback */
        DrawCircle((int)px, (int)py, raio, cor);
        /* Borda (mais grossa se destaque) */
        DrawCircleLines((int)px, (int)py, raio,
                        destaque ? (Color){255, 255, 255, 200} : (Color){30, 30, 60, 200});
        /* Inicial */
        char ini[2] = { j->nome[0], '\0' };
        int  fs = raio - 2;
        int  iw = MeasureText(ini, fs);
        DrawText(ini, (int)(px - iw/2), (int)(py - fs/2), fs, (Color){10, 20, 50, 255});
    }
}

/* ------------------------------------------------------------------ */
/* render_tabuleiro                                                     */
/* ------------------------------------------------------------------ */
Rectangle obter_rect_deck(int tipo_deck)
{
    float deck_w = 120;
    float deck_h = 80;
    float gap = 30;
    float total_w = 3 * deck_w + 2 * gap;
    float start_x = IMG_X + IMG_W / 2.0f - total_w / 2.0f;
    float start_y = IMG_Y + IMG_H / 2.0f - deck_h / 2.0f;
    
    return (Rectangle){start_x + tipo_deck * (deck_w + gap), start_y, deck_w, deck_h};
}

void render_tabuleiro(const Tabuleiro *tab,
                      const Jogador *jogadores, int num_jogadores, int jogador_atual,
                      const AnimacaoTurno *anim)
{
    if (!tab || !tab->cabeca) return;

    if (textura_tabuleiro.id != 0) {
        Rectangle src = {0, 0, (float)textura_tabuleiro.width, (float)textura_tabuleiro.height};
        Rectangle dst = {IMG_X, IMG_Y, IMG_W, IMG_H};
        DrawTexturePro(textura_tabuleiro, src, dst, (Vector2){0,0}, 0.0f, WHITE);
        
        /* Apenas desenha borda para propriedades compradas por cima da imagem */
        Casa *c = tab->cabeca;
        do {
            if (c->proprietario >= 0 && c->proprietario < num_jogadores) {
                Rectangle r = render_casa_rect(c->id);
                /* Ajusta a borda para ficar ligeiramente interna e não sobrepor as linhas do desenho */
                DrawRectangleLinesEx((Rectangle){r.x+2, r.y+2, r.width-4, r.height-4}, 4, COR_PINO[c->proprietario]);
            }
        } while ((c = c->next) != tab->cabeca);
        
        /* Desenha os 3 decks de cartas no centro */
        for (int i = 0; i < 3; i++) {
            Rectangle rect = obter_rect_deck(i);
            if (textura_costas_cartas[i].id != 0) {
                /* As imagens são 16:9 (2752x1536), mas o deck é 3:2 (120x80) horizontal. 
                 * Para não achatar, pegamos um crop de 2304x1536 no centro. */
                float tw = textura_costas_cartas[i].width;
                float th = textura_costas_cartas[i].height;
                float crop_w = th * (3.0f / 2.0f); /* Ex: 1536 * 1.5 = 2304 */
                float crop_x = (tw - crop_w) / 2.0f;
                Rectangle src = {crop_x, 0, crop_w, th};
                DrawTexturePro(textura_costas_cartas[i], src, rect, (Vector2){0,0}, 0.0f, WHITE);
                
                /* Destaque branco se passar o mouse por cima E for a vez de comprar dessa pilha */
                if (anim && anim->estado == TURNO_ESPERANDO_COMPRA_CARTA && jogadores && jogadores[jogador_atual].tipo == TIPO_HUMANO) {
                    int deck_ativo = -1;
                    if (jogadores[jogador_atual].posicao->tipo == CASA_SORTE) deck_ativo = 0;
                    if (jogadores[jogador_atual].posicao->tipo == CASA_AZAR)  deck_ativo = 1;
                    if (jogadores[jogador_atual].posicao->tipo == CASA_EVENTO) deck_ativo = 2;
                    
                    if (i == deck_ativo) {
                        Vector2 mouse = GetMousePosition();
                        if (CheckCollisionPointRec(mouse, rect)) {
                            DrawRectangleLinesEx((Rectangle){rect.x-4, rect.y-4, rect.width+8, rect.height+8}, 4, WHITE);
                        } else {
                            DrawRectangleLinesEx((Rectangle){rect.x-2, rect.y-2, rect.width+4, rect.height+4}, 2, (Color){255,255,255,100});
                        }
                    }
                }
            }
        }
    } else {
        /* Área central */
        DrawRectangle(BX+CS, BY+CS, 5*SW, 5*SH, (Color){8, 18, 38, 255});
        DrawText("REVITALIZA", BX+CS+10,  BY+CS+80,  28, (Color){255,140,30,50});
        DrawText("RECIFE",     BX+CS+60,  BY+CS+115, 28, (Color){255,140,30,50});

        /* Casas */
        Casa *c = tab->cabeca;
        do {
            Rectangle r   = render_casa_rect(c->id);
            Color     cor = cor_da_casa(c);

            DrawRectangleRec(r, (Color){10, 25, 55, 255});
            DrawRectangleRec(r, cor);

            /* Borda colorida com a cor do dono (ou borda padrão) */
            Color borda = (c->proprietario >= 0 && c->proprietario < num_jogadores)
                          ? COR_PINO[c->proprietario] : COR_BORDA;
            DrawRectangleLinesEx(r, 2, borda);

            char id_str[4];
            snprintf(id_str, sizeof(id_str), "%02d", c->id);
            DrawText(id_str, (int)r.x + 4, (int)r.y + 4, 9, (Color){255,255,255,160});

            desenhar_nome(c->nome, r, 8);
        } while ((c = c->next) != tab->cabeca);
    }

    /* --- Pinos de todos os jogadores --- */

    /* Calcula posição do pino do jogador atual (pode estar animado) */
    float cur_px = 0, cur_py = 0;
    if (anim && anim->estado == TURNO_PINO_MOVENDO &&
        jogadores && jogadores[jogador_atual].posicao) {
        float t = anim->timer_passo / DURACAO_PASSO;
        if (t > 1.0f) t = 1.0f;
        t = t * t * (3.0f - 2.0f * t);
        cur_px = anim->pos_inicio.x + (anim->pos_fim.x - anim->pos_inicio.x) * t;
        cur_py = anim->pos_inicio.y + (anim->pos_fim.y - anim->pos_inicio.y) * t;
        cur_py -= 14.0f * sinf(t * 3.14159f);  /* arco leve */
    } else if (jogadores && jogadores[jogador_atual].posicao) {
        Rectangle r = render_casa_rect(jogadores[jogador_atual].posicao->id);
        cur_px = r.x + r.width/2.0f + PINO_OX[jogador_atual];
        cur_py = r.y + r.height/2.0f + PINO_OY[jogador_atual];
    }

    /* Desenha jogadores não-atuais primeiro (menores, atrás) */
    for (int i = 0; i < num_jogadores; i++) {
        if (i == jogador_atual) continue;
        if (!jogadores[i].posicao) continue;
        Rectangle r = render_casa_rect(jogadores[i].posicao->id);
        float px = r.x + r.width/2.0f  + PINO_OX[i];
        float py = r.y + r.height/2.0f + PINO_OY[i];
        desenhar_pino(px, py, i, &jogadores[i], 10, 0);
    }

    /* Desenha jogador atual por cima (maior, com destaque) */
    if (jogadores && jogadores[jogador_atual].posicao) {
        desenhar_pino(cur_px, cur_py, jogador_atual, &jogadores[jogador_atual], 13, 1);
    }
}

/* ------------------------------------------------------------------ */
/* render_hud                                                           */
/* ------------------------------------------------------------------ */
static void barra_progresso(int x, int y, int w, int h, float ratio, Color cor)
{
    if (ratio > 1.0f) ratio = 1.0f;
    DrawRectangle(x, y, w, h, (Color){20, 30, 55, 255});
    DrawRectangle(x, y, (int)(w * ratio), h, cor);
    DrawRectangleLines(x, y, w, h, (Color){60, 80, 120, 255});
}

void render_hud(const Jogador *jogadores, int num_jogadores, int jogador_atual, int jogador_focado,
                int ultimo_dado)
{
    const int HX = 962, HW = 308;

    DrawRectangle(960, 0, 320, 720, (Color){8, 18, 38, 255});
    DrawLineEx((Vector2){960, 0}, (Vector2){960, 720}, 2, (Color){255,140,30,200});

    if (!jogadores) return;

    if (jogador_focado < 0 || jogador_focado >= num_jogadores)
        jogador_focado = jogador_atual;

    const Jogador *j = &jogadores[jogador_focado];
    int x = HX, y = 20;
    char buf[64];

    /* Título com cor do pino focado */
    DrawText("STATUS", x, y, 20, COR_PINO[jogador_focado]);
    y += 28;
    DrawLine(x, y, x+HW, y, (Color){255,140,30,80});
    y += 10;

    /* Nome e tipo do jogador atual */
    const char *tipo_str = (j->tipo == TIPO_BOT) ? " [BOT]" : "";
    snprintf(buf, sizeof(buf), "%s%s", j->nome, tipo_str);
    DrawText(buf, x, y, 16, COR_PINO[jogador_focado]);
    y += 28;

    /* Moedas */
    DrawText("MOEDAS", x, y, 11, (Color){160,160,160,255});
    y += 16;
    snprintf(buf, sizeof(buf), "$ %d", j->moedas);
    DrawText(buf, x, y, 22, (Color){255,215,0,255});
    y += 36;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 10;

    /* Barras de progresso por setor */
    DrawText("PONTOS DE IMPACTO", x, y, 11, (Color){160,160,160,255});
    y += 18;

    DrawText("TECNOLOGIA", x, y, 12, COR_TEC);
    snprintf(buf, sizeof(buf), "%d/%d", j->pontos[SETOR_TECNOLOGIA], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,12), y, 12, WHITE);
    y += 16;
    barra_progresso(x, y, HW, 9, (float)j->pontos[SETOR_TECNOLOGIA]/META_PONTOS, COR_TEC);
    y += 18;

    DrawText("TURISMO", x, y, 12, COR_TUR);
    snprintf(buf, sizeof(buf), "%d/%d", j->pontos[SETOR_TURISMO], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,12), y, 12, WHITE);
    y += 16;
    barra_progresso(x, y, HW, 9, (float)j->pontos[SETOR_TURISMO]/META_PONTOS, COR_TUR);
    y += 18;

    DrawText("COMERCIO", x, y, 12, COR_COM);
    snprintf(buf, sizeof(buf), "%d/%d", j->pontos[SETOR_COMERCIO], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,12), y, 12, WHITE);
    y += 16;
    barra_progresso(x, y, HW, 9, (float)j->pontos[SETOR_COMERCIO]/META_PONTOS, COR_COM);
    y += 24;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 10;

    /* Mini-lista de todos os jogadores */
    DrawText("JOGADORES", x, y, 11, (Color){160,160,160,255});
    y += 16;

    for (int i = 0; i < num_jogadores; i++) {
        int pts = jogadores[i].pontos[SETOR_TECNOLOGIA]
                + jogadores[i].pontos[SETOR_TURISMO]
                + jogadores[i].pontos[SETOR_COMERCIO];

        /* Destaque se for o turno atual */
        Color cor_nome = (i == jogador_atual)
                         ? COR_PINO[i]
                         : (Color){120, 140, 180, 180};

        /* Destacar o jogador que está sendo focado no HUD */
        if (i == jogador_focado) {
            DrawRectangle(x, y - 2, HW - 10, 20, (Color){30, 50, 80, 255});
        }

        /* Bolinha colorida */
        DrawCircle(x + 6, y + 7, 5, COR_PINO[i]);

        snprintf(buf, sizeof(buf), "%-10s  %2dpt  $%d",
                 jogadores[i].nome, pts, jogadores[i].moedas);
        DrawText(buf, x + 15, y, 11, cor_nome);

        /* Símbolo do turno atual */
        if (i == jogador_atual)
            DrawText(">", x - 8, y, 11, COR_PINO[i]);
            
        /* Se focado e não for o turno atual, indicar de leve */
        if (i == jogador_focado && i != jogador_atual)
            DrawText("*", x - 8, y, 11, (Color){180, 180, 180, 200});

        y += 20;
    }

    y += 6;
    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 10;

    /* Último dado */
    DrawText("ULTIMO DADO", x, y, 11, (Color){160,160,160,255});
    y += 16;
    if (ultimo_dado > 0) {
        snprintf(buf, sizeof(buf), "%d", ultimo_dado);
        DrawText(buf, x, y, 30, (Color){255,140,30,255});
    } else {
        DrawText("-", x, y, 30, (Color){100,100,100,255});
    }
    y += 44;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 10;

    DrawText("CONTROLES", x, y, 11, (Color){160,160,160,255});
    y += 16;
    if (j->tipo == TIPO_HUMANO) {
        if (j->turnos_bloqueado > 0) {
            snprintf(buf, sizeof(buf), "[ESPACO]  Pular turno (%d)", j->turnos_bloqueado);
            DrawText(buf, x, y, 11, (Color){220,80,80,180});
        } else {
            // Hack para descobrir se estamos esperando carta (só para UI visual do jogador atual)
            // Como render_hud não recebe anim, não temos como checar direto de forma limpa.
            // Vou manter genérico.
            DrawText("[ESPACO]  Rolar/Puxar/Avançar", x, y, 11, (Color){200,220,255,180});
        }
    } else {
        DrawText("Bot pensando...", x, y, 11, (Color){160,200,160,180});
    }
}

/* ------------------------------------------------------------------ */
/* render_dado                                                          */
/* ------------------------------------------------------------------ */

static const struct { int n; float x[6]; float y[6]; } FACES[7] = {
    {0},
    {1, {0.00f},                                      {0.00f}                                     },
    {2, { 0.32f, -0.32f},                             {-0.32f,  0.32f}                            },
    {3, { 0.32f,  0.00f, -0.32f},                     {-0.32f,  0.00f,  0.32f}                   },
    {4, {-0.32f,  0.32f, -0.32f,  0.32f},             {-0.32f, -0.32f,  0.32f,  0.32f}           },
    {5, {-0.32f,  0.32f,  0.00f, -0.32f,  0.32f},     {-0.32f, -0.32f,  0.00f,  0.32f,  0.32f}  },
    {6, {-0.32f,  0.32f, -0.32f,  0.32f, -0.32f,  0.32f},
        {-0.34f, -0.34f,  0.00f,  0.00f,  0.34f,  0.34f}                                         },
};

static void desenhar_pontos_dado(int face, float cx, float cy, float raio)
{
    if (face < 1 || face > 6) return;
    float dr = raio * 0.14f;
    for (int i = 0; i < FACES[face].n; i++) {
        DrawCircle(
            (int)(cx + FACES[face].x[i] * raio),
            (int)(cy + FACES[face].y[i] * raio),
            (int)dr,
            (Color){20, 30, 70, 255}
        );
    }
}

void render_dado_simples(int face, int cx, int cy, int raio, int jitter_x, int jitter_y, int destaque)
{
    int px = cx + jitter_x;
    int py = cy + jitter_y;

    DrawRectangleRounded(
        (Rectangle){(float)(px - raio + 5), (float)(py - raio + 5),
                    (float)(2*raio),         (float)(2*raio)},
        0.18f, 6, (Color){0, 0, 0, 100}
    );

    DrawRectangleRounded(
        (Rectangle){(float)(px - raio), (float)(py - raio),
                    (float)(2*raio),    (float)(2*raio)},
        0.18f, 6, (Color){240, 240, 250, 255}
    );

    Color borda = destaque ? (Color){255, 140, 30, 255} : (Color){180, 180, 200, 255};
    DrawRectangleRoundedLines(
        (Rectangle){(float)(px - raio), (float)(py - raio),
                    (float)(2*raio),    (float)(2*raio)},
        0.18f, 6, borda
    );

    desenhar_pontos_dado(face, (float)px, (float)py, (float)raio);
}

void render_dado(const AnimacaoTurno *anim)
{
    if (!anim || anim->estado == TURNO_AGUARDANDO) return;

    const int CX   = BX + CS + (5*SW) / 2;
    const int CY   = BY + CS + (5*SH) / 2;
    const int RAIO = 52;

    DrawRectangle(BX+CS, BY+CS, 5*SW, 5*SH, (Color){0, 0, 0, 150});

    render_dado_simples(anim->face_atual, CX, CY, RAIO, anim->jitter_x, anim->jitter_y, anim->estado == TURNO_PINO_MOVENDO);

    if (anim->estado == TURNO_PINO_MOVENDO) {
        char txt[24];
        snprintf(txt, sizeof(txt), "%d passo%s",
                 anim->passos_restantes,
                 anim->passos_restantes == 1 ? "" : "s");
        int tw = MeasureText(txt, 13);
        DrawText(txt, CX + anim->jitter_x - tw/2, CY + anim->jitter_y + RAIO + 10, 13, (Color){255,215,0,255});
    }
}

/* ------------------------------------------------------------------ */
/* render_carta_overlay                                                 */
/* ------------------------------------------------------------------ */

static int wrap_text(const char *text, int max_width, int font_size,
                     char lines[][128], int max_lines)
{
    int n = 0;
    const char *p = text;
    char cur[128] = {0};

    while (*p && n < max_lines) {
        const char *word = p;
        while (*p && *p != ' ') p++;

        char word_buf[64] = {0};
        int  wlen = (int)(p - word);
        if (wlen >= 63) wlen = 63;
        strncpy(word_buf, word, (size_t)wlen);

        char test[256];
        if (cur[0]) {
            snprintf(test, sizeof(test), "%s %s", cur, word_buf);
        } else {
            snprintf(test, sizeof(test), "%s", word_buf);
        }

        if (MeasureText(test, font_size) > max_width && cur[0]) {
            strncpy(lines[n++], cur, 127);
            strncpy(cur, word_buf, 127);
        } else {
            strncpy(cur, test, 127);
        }

        if (*p == ' ') p++;
    }
    if (cur[0] && n < max_lines)
        strncpy(lines[n++], cur, 127);

    return n;
}

static void efeito_resumo(const Carta *c, char *buf, int len)
{
    static const char *snomes[] = {"", "TECNOLOGIA", "TURISMO", "COMERCIO"};
    switch (c->efeito) {
        case EFEITO_GANHAR_PONTOS:
            snprintf(buf, len, "+%d pontos de %s", c->valor, snomes[c->setor]); break;
        case EFEITO_PERDER_PONTOS:
            snprintf(buf, len, "-%d pontos de %s", c->valor, snomes[c->setor]); break;
        case EFEITO_GANHAR_MOEDAS:
            snprintf(buf, len, "+%d moedas",  c->valor); break;
        case EFEITO_PERDER_MOEDAS:
            snprintf(buf, len, "-%d moedas",  c->valor); break;
        case EFEITO_AVANCAR:
            snprintf(buf, len, "Avance %d casa%s", c->valor, c->valor==1?"":"s"); break;
        case EFEITO_VOLTAR_INICIO:
            snprintf(buf, len, "Volte ao Marco Zero"); break;
        case EFEITO_PERDER_TURNO:
            snprintf(buf, len, "Perde %d turno%s", c->valor, c->valor==1?"":"s"); break;
        case EFEITO_TODOS_GANHAM_PONTOS:
            snprintf(buf, len, "Todos ganham +%d pt %s", c->valor, snomes[c->setor]); break;
        case EFEITO_TODOS_PERDEM_MOEDAS:
            snprintf(buf, len, "Todos pagam %d moedas", c->valor); break;
        case EFEITO_PROPRIETARIOS_BONUS:
            snprintf(buf, len, "Donos de %s ganham +%d pt", snomes[c->setor], c->valor); break;
        default:
            snprintf(buf, len, "Efeito desconhecido");
    }
}

void render_carta_overlay(const AnimacaoTurno *anim)
{
    if (!anim) return;

    if (anim->estado == TURNO_ANIMANDO_COMPRA_CARTA && anim->carta_ativa) {
        /* Animação da carta voando do deck para o centro da tela */
        int deck_idx = -1;
        if (anim->carta_ativa->tipo == CARTA_SORTE) deck_idx = 0;
        if (anim->carta_ativa->tipo == CARTA_AZAR)  deck_idx = 1;
        if (anim->carta_ativa->tipo == CARTA_EVENTO) deck_idx = 2;

        if (deck_idx != -1 && textura_costas_cartas[deck_idx].id != 0) {
            Rectangle deck_rect = obter_rect_deck(deck_idx);
            /* Carta voando ganha escala mas preserva o ratio 3:2 horizontal */
            const int PW_CARD = 375, PH_CARD = 250;
            /* Centro do tabuleiro: X = 10 + 938/2 = 479, Y = 104 + 511/2 = 360 */
            const int PX = 479 - PW_CARD / 2;
            const int PY = 360 - PH_CARD / 2;
            
            float t = anim->timer_carta; /* de 0.0 a 1.0 */
            /* Easing out simples (desacelera no final) */
            float ease_t = 1.0f - (1.0f - t) * (1.0f - t);
            
            float curr_x = deck_rect.x + (PX - deck_rect.x) * ease_t;
            float curr_y = deck_rect.y + (PY - deck_rect.y) * ease_t;
            float curr_w = deck_rect.width + (PW_CARD - deck_rect.width) * ease_t;
            float curr_h = deck_rect.height + (PH_CARD - deck_rect.height) * ease_t;
            
            /* Escurecer fundo proporcionalmente ao tempo */
            DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, (unsigned char)(170 * ease_t)});
            
            float tw = textura_costas_cartas[deck_idx].width;
            float th = textura_costas_cartas[deck_idx].height;
            float crop_w = th * (3.0f / 2.0f);
            float crop_x = (tw - crop_w) / 2.0f;
            Rectangle src = {crop_x, 0, crop_w, th};
            
            Rectangle dst = {curr_x, curr_y, curr_w, curr_h};
            DrawTexturePro(textura_costas_cartas[deck_idx], src, dst, (Vector2){0,0}, 0.0f, WHITE);
        }
        return;
    }

    if (anim->estado != TURNO_MOSTRANDO_CARTA || !anim->carta_ativa)
        return;

    const Carta *c = anim->carta_ativa;

    Color cor_tipo, cor_borda;
    const char *tipo_str;
    switch (c->tipo) {
        case CARTA_SORTE:
            cor_tipo  = (Color){ 50, 200,  80, 255};
            cor_borda = (Color){ 50, 200,  80, 255};
            tipo_str  = "SORTE";
            break;
        case CARTA_AZAR:
            cor_tipo  = (Color){220,  60,  60, 255};
            cor_borda = (Color){220,  60,  60, 255};
            tipo_str  = "AZAR";
            break;
        default:
            cor_tipo  = (Color){160, 100, 220, 255};
            cor_borda = (Color){160, 100, 220, 255};
            tipo_str  = "EVENTO";
            break;
    }

    const int PW = 500, PH = 380;
    /* Centro do tabuleiro é 479, 360 */
    const int PX = 479 - PW / 2;
    const int PY = 360 - PH / 2;

    /* Escurece a tela toda para focar na janela do evento */
    DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 170});

    DrawRectangleRounded(
        (Rectangle){(float)(PX+6), (float)(PY+6), (float)PW, (float)PH},
        0.06f, 8, (Color){0, 0, 0, 120});

    DrawRectangleRounded(
        (Rectangle){(float)PX, (float)PY, (float)PW, (float)PH},
        0.06f, 8, (Color){12, 22, 48, 255});

    DrawRectangleRoundedLines(
        (Rectangle){(float)PX, (float)PY, (float)PW, (float)PH},
        0.06f, 8, cor_borda);

    DrawRectangleRounded(
        (Rectangle){(float)PX, (float)PY, (float)PW, 64.0f},
        0.06f, 8, cor_tipo);
    DrawRectangle(PX, PY + 44, PW, 20, cor_tipo);

    int tw = MeasureText(tipo_str, 26);
    DrawText(tipo_str, PX + PW/2 - tw/2, PY + 18, 26, WHITE);

    int ty = PY + 80;
    tw = MeasureText(c->titulo, 22);
    DrawText(c->titulo, PX + PW/2 - tw/2, ty, 22, WHITE);

    ty += 36;
    DrawLine(PX + 20, ty, PX + PW - 20, ty, (Color){60, 80, 120, 255});
    ty += 14;

    char linhas[4][128] = {{0}};
    int  n = wrap_text(c->descricao, PW - 60, 14, linhas, 4);
    for (int i = 0; i < n; i++) {
        tw = MeasureText(linhas[i], 14);
        DrawText(linhas[i], PX + PW/2 - tw/2, ty, 14, (Color){180, 210, 255, 220});
        ty += 20;
    }

    ty += 6;
    DrawLine(PX + 20, ty, PX + PW - 20, ty, (Color){60, 80, 120, 255});
    ty += 16;

    char resumo[128];
    efeito_resumo(c, resumo, sizeof(resumo));
    tw = MeasureText(resumo, 18);
    DrawText(resumo, PX + PW/2 - tw/2, ty, 18, cor_tipo);

    const char *hint = "[ESPACO] ou [ENTER]  Continuar";
    tw = MeasureText(hint, 13);
    DrawText(hint, PX + PW/2 - tw/2, PY + PH - 34, 13,
             (Color){130, 150, 190, 200});
}

/* ------------------------------------------------------------------ */
/* render_propriedade_overlay                                           */
/* ------------------------------------------------------------------ */
void render_propriedade_overlay(const AnimacaoTurno *anim, const Jogador *jogador)
{
    if (!anim || anim->estado != TURNO_MOSTRANDO_PROPRIEDADE || !anim->casa_ativa)
        return;

    const Casa *casa = anim->casa_ativa;

    Color cor_setor;
    const char *setor_str;
    switch (casa->setor) {
        case SETOR_TECNOLOGIA: cor_setor = COR_TEC; setor_str = "TECNOLOGIA"; break;
        case SETOR_TURISMO:    cor_setor = COR_TUR; setor_str = "TURISMO";    break;
        case SETOR_COMERCIO:   cor_setor = COR_COM; setor_str = "COMERCIO";   break;
        default:               cor_setor = (Color){150,150,150,255}; setor_str = "NEUTRO"; break;
    }

    const int PW = 480, PH = 360;
    const int PX = (1280 - PW) / 2;
    const int PY = (720  - PH) / 2;

    DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 170});

    DrawRectangleRounded(
        (Rectangle){(float)(PX+6),(float)(PY+6),(float)PW,(float)PH},
        0.06f, 8, (Color){0,0,0,120});

    DrawRectangleRounded(
        (Rectangle){(float)PX,(float)PY,(float)PW,(float)PH},
        0.06f, 8, (Color){12,22,48,255});

    DrawRectangleRoundedLines(
        (Rectangle){(float)PX,(float)PY,(float)PW,(float)PH},
        0.06f, 8, cor_setor);

    DrawRectangleRounded(
        (Rectangle){(float)PX,(float)PY,(float)PW,60.0f},
        0.06f, 8, cor_setor);
    DrawRectangle(PX, PY+40, PW, 20, cor_setor);

    const char *header = anim->eh_aluguel ? "ALUGUEL" : "PROPRIEDADE A VENDA";
    int tw = MeasureText(header, 22);
    DrawText(header, PX + PW/2 - tw/2, PY + 17, 22, WHITE);

    int ty = PY + 76;
    char buf[64];

    tw = MeasureText(casa->nome, 20);
    DrawText(casa->nome, PX + PW/2 - tw/2, ty, 20, WHITE);
    ty += 30;

    int sw = MeasureText(setor_str, 12);
    DrawRectangle(PX + PW/2 - sw/2 - 8, ty - 2, sw + 16, 20, cor_setor);
    DrawText(setor_str, PX + PW/2 - sw/2, ty, 12, WHITE);
    ty += 30;

    DrawLine(PX+20, ty, PX+PW-20, ty, (Color){60,80,120,255});
    ty += 16;

    if (anim->eh_aluguel) {
        int aluguel = casa->custo / 2;
        snprintf(buf, sizeof(buf), "Aluguel: %d moedas", aluguel);
        tw = MeasureText(buf, 20);
        DrawText(buf, PX + PW/2 - tw/2, ty, 20, (Color){255,215,0,255});
        ty += 32;

        snprintf(buf, sizeof(buf), "Seu saldo: %d moedas", jogador->moedas);
        tw = MeasureText(buf, 14);
        Color cs = (jogador->moedas >= aluguel)
                   ? (Color){180,210,255,200} : (Color){220,60,60,255};
        DrawText(buf, PX + PW/2 - tw/2, ty, 14, cs);

        const char *hint = "[ESPACO] ou [ENTER]  Pagar aluguel";
        tw = MeasureText(hint, 13);
        DrawText(hint, PX + PW/2 - tw/2, PY + PH - 34, 13,
                 (Color){130,150,190,200});
    } else {
        snprintf(buf, sizeof(buf), "Custo: %d moedas", casa->custo);
        tw = MeasureText(buf, 20);
        DrawText(buf, PX + PW/2 - tw/2, ty, 20, (Color){255,215,0,255});
        ty += 28;

        snprintf(buf, sizeof(buf), "Gera: +%d pontos de %s", casa->pontos, setor_str);
        tw = MeasureText(buf, 14);
        DrawText(buf, PX + PW/2 - tw/2, ty, 14, cor_setor);
        ty += 26;

        snprintf(buf, sizeof(buf), "Seu saldo: %d moedas", jogador->moedas);
        tw = MeasureText(buf, 14);
        Color cs = (jogador->moedas >= casa->custo)
                   ? (Color){180,210,255,200} : (Color){220,60,60,255};
        DrawText(buf, PX + PW/2 - tw/2, ty, 14, cs);
        ty += 32;

        DrawLine(PX+20, ty, PX+PW-20, ty, (Color){60,80,120,255});
        ty += 18;

        int pode = (jogador->moedas >= casa->custo);
        Color cor_c     = pode ? (Color){30,130,60,255} : (Color){40,60,45,255};
        Color cor_c_txt = pode ? WHITE                  : (Color){80,110,85,255};

        DrawRectangleRounded(
            (Rectangle){(float)(PX+50),(float)ty,160.0f,44.0f},
            0.22f, 6, cor_c);
        const char *c_txt = "[C]  Comprar";
        tw = MeasureText(c_txt, 16);
        DrawText(c_txt, PX+50+80-tw/2, ty+14, 16, cor_c_txt);

        DrawRectangleRounded(
            (Rectangle){(float)(PX+PW-210),(float)ty,160.0f,44.0f},
            0.22f, 6, (Color){90,30,30,255});
        const char *x_txt = "[X]  Passar";
        tw = MeasureText(x_txt, 16);
        DrawText(x_txt, PX+PW-210+80-tw/2, ty+14, 16, WHITE);

        if (!pode) {
            const char *nota = "Moedas insuficientes";
            tw = MeasureText(nota, 12);
            DrawText(nota, PX+PW/2-tw/2, PY+PH-34, 12, (Color){220,80,80,200});
        }
    }
}

Rectangle obter_rect_carta_acao(int slot, int hover) {
    float cw = 120, ch = 180; 
    float x = 200 + slot * 140; 
    float y = hover ? 720 - ch + 40 : 720 - 60; 
    return (Rectangle){x, y, cw, ch};
}

void render_hud_cartas_acao(Jogador *j, AnimacaoTurno *anim, Font fonte) {
    if (anim->estado != TURNO_AGUARDANDO && anim->estado != TURNO_USANDO_ACAO) return;
    if (j->tipo != TIPO_HUMANO) return;
    
    Vector2 mouse = GetMousePosition();
    for (int i=0; i<2; i++) {
        int c_id = j->cartas_acao[i];
        if (c_id == -1) continue;
        
        Rectangle rect = obter_rect_carta_acao(i, 0);
        int hover = CheckCollisionPointRec(mouse, rect);
        if (hover || (anim->estado == TURNO_USANDO_ACAO && anim->acao_slot == i)) {
            rect = obter_rect_carta_acao(i, 1);
            hover = 1;
        }
        
        Texture2D tex = textura_cartas_acao[c_id];
        if (tex.id != 0) {
            float crop_w = tex.height * (2.0f/3.0f);
            Rectangle src = { (tex.width - crop_w)/2.0f, 0, crop_w, tex.height };
            DrawTexturePro(tex, src, rect, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(rect, DARKGRAY);
        }
        
        DrawRectangleLinesEx(rect, 3, hover ? WHITE : (Color){100,100,100,255});
        
        if (hover && anim->estado == TURNO_AGUARDANDO) {
            DrawText(TextFormat("[ %d ]", i+1), rect.x + rect.width/2 - 15, rect.y - 25, 20, WHITE);
        }
    }
}

void render_acao_overlay(Jogador *jogadores, int num_jogadores, int jogador_atual, AnimacaoTurno *anim, Font fonte) {
    if (anim->estado != TURNO_USANDO_ACAO) return;
    
    DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 200});
    
    int c_id = anim->acao_carta_id;
    Texture2D tex = textura_cartas_acao[c_id];
    
    float cw = 280, ch = 420;
    float cx = 479 - cw/2, cy = 360 - ch/2;
    Rectangle rect = {cx, cy, cw, ch};
    
    if (tex.id != 0) {
        float crop_w = tex.height * (2.0f/3.0f);
        Rectangle src = { (tex.width - crop_w)/2.0f, 0, crop_w, tex.height };
        DrawTexturePro(tex, src, rect, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(rect, DARKGRAY);
    }
    
    Rectangle box = { cx + cw + 40, cy + 50, 420, 320 };
    DrawRectangleRounded(box, 0.1f, 10, (Color){40, 40, 60, 240});
    DrawRectangleLinesEx(box, 3, WHITE);
    
    const char *title = "";
    const char *desc = "";
    if (c_id == 0) {
        title = "Ação de Turismo";
        desc = "Ganha +1 ponto de turismo.\nGanha +1 adicional para cada\npropriedade de turismo.";
    } else if (c_id == 1) {
        title = "Recife em Festa";
        desc = "Pelos próximos 2 turnos,\ntodos os seus pontos ganhos\nsão dobrados!";
    } else if (c_id == 2) {
        title = "Ação de Comércio";
        desc = "Ganha +1 ponto de comércio.\nGanha +1 adicional para cada\npropriedade de comércio.";
    } else if (c_id == 3) {
        title = "Parceria Estratégica";
        desc = "Escolha um jogador parceiro.\nAmbos ganham +1 em tudo.\n+1 bônus extra se tiverem\nsetores diferentes!";
    }
    
    DrawTextEx(fonte, title, (Vector2){box.x + 20, box.y + 20}, 24, 1, WHITE);
    DrawTextEx(fonte, desc, (Vector2){box.x + 20, box.y + 60}, 20, 1, LIGHTGRAY);
    
    if (c_id == 3) {
        DrawTextEx(fonte, "Escolha um alvo:", (Vector2){box.x + 20, box.y + 150}, 20, 1, GOLD);
        int y_offset = 180;
        for (int i=0; i<num_jogadores; i++) {
            if (i == jogador_atual) continue;
            Color c = (anim->acao_alvo_idx == i) ? (Color){40,160,80,255} : (Color){60,60,80,255};
            DrawRectangleRounded((Rectangle){box.x + 20, box.y + y_offset, 380, 30}, 0.2f, 5, c);
            DrawText(TextFormat("[%d] %s", i+1, jogadores[i].nome), box.x + 30, box.y + y_offset + 5, 20, WHITE);
            y_offset += 40;
        }
    }
    
    DrawRectangleRounded((Rectangle){box.x + 20, box.y + 260, 180, 40}, 0.2f, 5, (Color){40,160,80,255});
    DrawText("[ENTER] Confirmar", box.x + 35, box.y + 270, 16, WHITE);
    
    DrawRectangleRounded((Rectangle){box.x + 220, box.y + 260, 180, 40}, 0.2f, 5, (Color){160,40,40,255});
    DrawText("[ESC] Cancelar", box.x + 255, box.y + 270, 16, WHITE);
}
