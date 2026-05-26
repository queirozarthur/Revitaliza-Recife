#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Geometria do tabuleiro (ver diagrama em render.h)                   */
/* ------------------------------------------------------------------ */
#define BX  20
#define BY  20
#define CS 110
#define SW 140
#define SH  88

/* Paleta */
#define COR_TEC   (Color){ 30, 144, 255, 220}
#define COR_TUR   (Color){ 60, 179, 113, 220}
#define COR_COM   (Color){255, 165,   0, 220}
#define COR_INI   (Color){255, 215,   0, 220}
#define COR_SOR   (Color){ 50, 205,  50, 220}
#define COR_AZA   (Color){210,  50,  50, 220}
#define COR_EVT   (Color){160, 100, 220, 220}
#define COR_BORDA (Color){ 70,  90, 130, 255}
#define COR_PROP  (Color){ 30,  80, 130, 255}

/* ------------------------------------------------------------------ */
/* Posicionamento das casas                                             */
/* ------------------------------------------------------------------ */
Rectangle render_casa_rect(int id)
{
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

/* ------------------------------------------------------------------ */
/* render_tabuleiro                                                     */
/* ------------------------------------------------------------------ */
void render_tabuleiro(const Tabuleiro *tab, const Jogador *jogador,
                      const AnimacaoTurno *anim)
{
    if (!tab || !tab->cabeca) return;

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

        Color borda = (c->proprietario >= 0) ? WHITE : COR_BORDA;
        DrawRectangleLinesEx(r, 2, borda);

        char id_str[4];
        snprintf(id_str, sizeof(id_str), "%02d", c->id);
        DrawText(id_str, (int)r.x + 4, (int)r.y + 4, 9, (Color){255,255,255,160});

        desenhar_nome(c->nome, r, 8);
    } while ((c = c->next) != tab->cabeca);

    /* --- Pino com lerp suavizado --- */
    if (!jogador || !jogador->posicao) return;

    float px, py;

    if (anim && anim->estado == TURNO_PINO_MOVENDO) {
        /* Smoothstep: t³(6t²−15t+10)... usamos cúbica simples t²(3−2t) */
        float t = anim->timer_passo / DURACAO_PASSO;
        if (t > 1.0f) t = 1.0f;
        t = t * t * (3.0f - 2.0f * t);

        px = anim->pos_inicio.x + (anim->pos_fim.x - anim->pos_inicio.x) * t;
        py = anim->pos_inicio.y + (anim->pos_fim.y - anim->pos_inicio.y) * t;

        /* Arco leve: sobe no meio do passo */
        py -= 14.0f * sinf(t * 3.14159f);
    } else {
        Rectangle r = render_casa_rect(jogador->posicao->id);
        px = r.x + r.width  / 2.0f;
        py = r.y + r.height / 2.0f;
    }

    /* Sombra */
    DrawCircle((int)px + 2, (int)py + 2, 14, (Color){0, 0, 0, 120});
    /* Corpo */
    DrawCircle((int)px,     (int)py,     14, WHITE);
    DrawCircleLines((int)px, (int)py,    14, (Color){30, 30, 60, 255});
    /* Inicial */
    char ini[2] = { jogador->nome[0], '\0' };
    int  iw = MeasureText(ini, 14);
    DrawText(ini, (int)(px - iw/2), (int)(py - 7), 14, (Color){10, 20, 50, 255});
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

void render_hud(const Jogador *jogador, int ultimo_dado)
{
    const int HX = 962, HW = 308;

    DrawRectangle(960, 0, 320, 720, (Color){8, 18, 38, 255});
    DrawLineEx((Vector2){960, 0}, (Vector2){960, 720}, 2, (Color){255,140,30,200});

    if (!jogador) return;

    int x = HX, y = 20;
    char buf[64];

    DrawText("STATUS", x, y, 20, (Color){255,140,30,255});
    y += 28;
    DrawLine(x, y, x+HW, y, (Color){255,140,30,80});
    y += 14;

    DrawText(jogador->nome, x, y, 18, (Color){220,235,255,255});
    y += 36;

    DrawText("MOEDAS", x, y, 11, (Color){160,160,160,255});
    y += 16;
    snprintf(buf, sizeof(buf), "$ %d", jogador->moedas);
    DrawText(buf, x, y, 24, (Color){255,215,0,255});
    y += 42;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 14;

    DrawText("PONTOS DE IMPACTO", x, y, 11, (Color){160,160,160,255});
    y += 20;

    DrawText("TECNOLOGIA", x, y, 13, COR_TEC);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_TECNOLOGIA], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_TECNOLOGIA]/META_PONTOS, COR_TEC);
    y += 22;

    DrawText("TURISMO", x, y, 13, COR_TUR);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_TURISMO], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_TURISMO]/META_PONTOS, COR_TUR);
    y += 22;

    DrawText("COMERCIO", x, y, 13, COR_COM);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_COMERCIO], META_PONTOS);
    DrawText(buf, x+HW-MeasureText(buf,13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_COMERCIO]/META_PONTOS, COR_COM);
    y += 32;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 14;

    DrawText("POSICAO ATUAL", x, y, 11, (Color){160,160,160,255});
    y += 18;
    if (jogador->posicao) {
        DrawText(jogador->posicao->nome, x, y, 13, (Color){220,235,255,255});
        y += 20;
        Color sc = (Color){160,160,160,255};
        const char *ss = "NEUTRO";
        switch (jogador->posicao->setor) {
            case SETOR_TECNOLOGIA: sc = COR_TEC; ss = "TECNOLOGIA"; break;
            case SETOR_TURISMO:    sc = COR_TUR; ss = "TURISMO";    break;
            case SETOR_COMERCIO:   sc = COR_COM; ss = "COMERCIO";   break;
            default: break;
        }
        DrawText(ss, x, y, 12, sc);
    }
    y += 36;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 14;

    DrawText("ULTIMO DADO", x, y, 11, (Color){160,160,160,255});
    y += 18;
    if (ultimo_dado > 0) {
        snprintf(buf, sizeof(buf), "%d", ultimo_dado);
        DrawText(buf, x, y, 32, (Color){255,140,30,255});
    } else {
        DrawText("-", x, y, 32, (Color){100,100,100,255});
    }
    y += 56;

    DrawLine(x, y, x+HW, y, (Color){255,140,30,50});
    y += 14;

    DrawText("CONTROLES", x, y, 11, (Color){160,160,160,255});
    y += 18;
    DrawText("[ESPACO]  Rolar dado", x, y, 12, (Color){200,220,255,180});
}

/* ------------------------------------------------------------------ */
/* render_dado                                                          */
/* ------------------------------------------------------------------ */

/* Posições dos pontos em frações do raio do dado, por face */
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

void render_dado(const AnimacaoTurno *anim)
{
    if (!anim || anim->estado == TURNO_AGUARDANDO) return;

    /* Centro da área interna do tabuleiro */
    const int CX   = BX + CS + (5*SW) / 2;   /* 480 */
    const int CY   = BY + CS + (5*SH) / 2;   /* 350 */
    const int RAIO = 52;

    int cx = CX + anim->jitter_x;
    int cy = CY + anim->jitter_y;

    /* Overlay escuro atrás do dado */
    DrawRectangle(BX+CS, BY+CS, 5*SW, 5*SH, (Color){0, 0, 0, 150});

    /* Sombra */
    DrawRectangleRounded(
        (Rectangle){(float)(cx - RAIO + 5), (float)(cy - RAIO + 5),
                    (float)(2*RAIO),         (float)(2*RAIO)},
        0.18f, 6, (Color){0, 0, 0, 100}
    );

    /* Corpo do dado */
    DrawRectangleRounded(
        (Rectangle){(float)(cx - RAIO), (float)(cy - RAIO),
                    (float)(2*RAIO),    (float)(2*RAIO)},
        0.18f, 6, (Color){240, 240, 250, 255}
    );

    /* Borda laranja quando parado (resultado fixo) */
    Color borda = (anim->estado == TURNO_PINO_MOVENDO)
                  ? (Color){255, 140, 30, 255}
                  : (Color){180, 180, 200, 255};
    DrawRectangleRoundedLines(
        (Rectangle){(float)(cx - RAIO), (float)(cy - RAIO),
                    (float)(2*RAIO),    (float)(2*RAIO)},
        0.18f, 6, borda
    );

    /* Pontos da face */
    desenhar_pontos_dado(anim->face_atual, (float)cx, (float)cy, (float)RAIO);

    /* Quando o pino está movendo: mostra resultado + passos restantes */
    if (anim->estado == TURNO_PINO_MOVENDO) {
        char txt[24];
        snprintf(txt, sizeof(txt), "%d passo%s",
                 anim->passos_restantes,
                 anim->passos_restantes == 1 ? "" : "s");
        int tw = MeasureText(txt, 13);
        DrawText(txt, cx - tw/2, cy + RAIO + 10, 13, (Color){255,215,0,255});
    }
}

/* ------------------------------------------------------------------ */
/* render_carta_overlay                                                 */
/* ------------------------------------------------------------------ */

/* Quebra texto em linhas respeitando max_width */
static int wrap_text(const char *text, int max_width, int font_size,
                     char lines[][128], int max_lines)
{
    int n = 0;
    const char *p = text;
    char cur[128] = {0};

    while (*p && n < max_lines) {
        /* Acumula palavras até ultrapassar a largura */
        const char *word = p;
        while (*p && *p != ' ') p++;

        char word_buf[64] = {0};
        int  wlen = (int)(p - word);
        if (wlen >= 63) wlen = 63;
        strncpy(word_buf, word, (size_t)wlen);

        char test[256];   /* amplo o suficiente para cur+space+word */
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
    if (!anim || anim->estado != TURNO_MOSTRANDO_CARTA || !anim->carta_ativa)
        return;

    const Carta *c = anim->carta_ativa;

    /* Cores por tipo */
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
        default: /* EVENTO */
            cor_tipo  = (Color){160, 100, 220, 255};
            cor_borda = (Color){160, 100, 220, 255};
            tipo_str  = "EVENTO";
            break;
    }

    /* Dimensões do painel */
    const int PW = 500, PH = 380;
    const int PX = (1280 - PW) / 2;
    const int PY = (720  - PH) / 2;

    /* Overlay escuro */
    DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 170});

    /* Sombra do painel */
    DrawRectangleRounded(
        (Rectangle){(float)(PX+6), (float)(PY+6), (float)PW, (float)PH},
        0.06f, 8, (Color){0, 0, 0, 120});

    /* Fundo do painel */
    DrawRectangleRounded(
        (Rectangle){(float)PX, (float)PY, (float)PW, (float)PH},
        0.06f, 8, (Color){12, 22, 48, 255});

    /* Borda colorida */
    DrawRectangleRoundedLines(
        (Rectangle){(float)PX, (float)PY, (float)PW, (float)PH},
        0.06f, 8, cor_borda);

    /* Cabeçalho colorido */
    DrawRectangleRounded(
        (Rectangle){(float)PX, (float)PY, (float)PW, 64.0f},
        0.06f, 8, cor_tipo);
    /* Cobre cantos inferiores do header para ficar reto embaixo */
    DrawRectangle(PX, PY + 44, PW, 20, cor_tipo);

    int tw = MeasureText(tipo_str, 26);
    DrawText(tipo_str, PX + PW/2 - tw/2, PY + 18, 26, WHITE);

    /* Título */
    int ty = PY + 80;
    tw = MeasureText(c->titulo, 22);
    DrawText(c->titulo, PX + PW/2 - tw/2, ty, 22, WHITE);

    /* Linha separadora */
    ty += 36;
    DrawLine(PX + 20, ty, PX + PW - 20, ty, (Color){60, 80, 120, 255});
    ty += 14;

    /* Descrição (com quebra de linha) */
    char linhas[4][128] = {{0}};
    int  n = wrap_text(c->descricao, PW - 60, 14, linhas, 4);
    for (int i = 0; i < n; i++) {
        tw = MeasureText(linhas[i], 14);
        DrawText(linhas[i], PX + PW/2 - tw/2, ty, 14, (Color){180, 210, 255, 220});
        ty += 20;
    }

    /* Linha separadora */
    ty += 6;
    DrawLine(PX + 20, ty, PX + PW - 20, ty, (Color){60, 80, 120, 255});
    ty += 16;

    /* Efeito */
    char resumo[128];
    efeito_resumo(c, resumo, sizeof(resumo));
    tw = MeasureText(resumo, 18);
    DrawText(resumo, PX + PW/2 - tw/2, ty, 18, cor_tipo);

    /* Rodapé */
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

    /* Cabeçalho */
    DrawRectangleRounded(
        (Rectangle){(float)PX,(float)PY,(float)PW,60.0f},
        0.06f, 8, cor_setor);
    DrawRectangle(PX, PY+40, PW, 20, cor_setor);

    const char *header = anim->eh_aluguel ? "ALUGUEL" : "PROPRIEDADE A VENDA";
    int tw = MeasureText(header, 22);
    DrawText(header, PX + PW/2 - tw/2, PY + 17, 22, WHITE);

    int ty = PY + 76;
    char buf[64];

    /* Nome da propriedade */
    tw = MeasureText(casa->nome, 20);
    DrawText(casa->nome, PX + PW/2 - tw/2, ty, 20, WHITE);
    ty += 30;

    /* Badge do setor */
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

        /* Botão [C] */
        DrawRectangleRounded(
            (Rectangle){(float)(PX+50),(float)ty,160.0f,44.0f},
            0.22f, 6, cor_c);
        const char *c_txt = "[C]  Comprar";
        tw = MeasureText(c_txt, 16);
        DrawText(c_txt, PX+50+80-tw/2, ty+14, 16, cor_c_txt);

        /* Botão [X] */
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
