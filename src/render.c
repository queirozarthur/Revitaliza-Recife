#include "render.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Geometria do tabuleiro                                              */
/*                                                                     */
/*  Janela: 1280 x 720                                                 */
/*  Tabuleiro: x=20..940, y=20..680   (920 x 660)                     */
/*  HUD: x=960..1270                                                   */
/*                                                                     */
/*  Distribuição das 24 casas (sentido horário, partindo do BL):       */
/*                                                                     */
/*  [18][17][16][15][14][13][12]   <- topo   (D→E)                    */
/*  [19]                    [11]                                       */
/*  [20]       centro       [10]   <- lados                            */
/*  [21]                    [ 9]                                       */
/*  [22]                    [ 8]                                       */
/*  [23]                    [ 7]                                       */
/*  [ 0][ 1][ 2][ 3][ 4][ 5][ 6]  <- base   (E→D)                    */
/* ------------------------------------------------------------------ */

#define BX  20     /* board left x            */
#define BY  20     /* board top  y            */
#define CS  110    /* corner square size       */
#define SW  140    /* horizontal square width  */
#define SH  88     /* vertical   square height */

/* Paleta de cores por setor/tipo */
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
/* Posicionamento                                                       */
/* ------------------------------------------------------------------ */

Rectangle render_casa_rect(int id)
{
    /* cantos */
    if (id ==  0) return (Rectangle){BX,           BY + CS + 5*SH, CS, CS};
    if (id ==  6) return (Rectangle){BX + CS+5*SW, BY + CS + 5*SH, CS, CS};
    if (id == 12) return (Rectangle){BX + CS+5*SW, BY,             CS, CS};
    if (id == 18) return (Rectangle){BX,           BY,             CS, CS};

    /* base (E → D): IDs 1–5 */
    if (id >= 1 && id <= 5)
        return (Rectangle){BX + CS + (id-1)*SW, BY + CS + 5*SH, SW, CS};

    /* direita (B → T): IDs 7–11 */
    if (id >= 7 && id <= 11)
        return (Rectangle){BX + CS + 5*SW, BY + CS + (11-id)*SH, CS, SH};

    /* topo (D → E): IDs 13–17 */
    if (id >= 13 && id <= 17)
        return (Rectangle){BX + CS + (17-id)*SW, BY, SW, CS};

    /* esquerda (T → B): IDs 19–23 */
    if (id >= 19 && id <= 23)
        return (Rectangle){BX, BY + CS + (id-19)*SH, CS, SH};

    return (Rectangle){0, 0, 0, 0};
}

/* ------------------------------------------------------------------ */
/* Helpers internos                                                     */
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

/* Desenha nome da casa em até duas linhas dentro do retângulo r */
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

    /* Quebra na última palavra que cabe na primeira linha */
    int split = max_chars;
    while (split > 0 && nome[split] != ' ') split--;
    if (split == 0) split = max_chars;

    char linha1[MAX_NOME_CASA] = {0};
    strncpy(linha1, nome, (size_t)split);

    int tw1 = MeasureText(linha1, font);
    int tw2 = MeasureText(nome + split + 1, font);
    int cy  = (int)(r.y + r.height/2) - font;

    DrawText(linha1,
             (int)(r.x + r.width/2 - tw1/2), cy, font, WHITE);
    DrawText(nome + split + 1,
             (int)(r.x + r.width/2 - tw2/2), cy + font + 2, font, WHITE);
}

/* ------------------------------------------------------------------ */
/* Render: tabuleiro                                                    */
/* ------------------------------------------------------------------ */

void render_tabuleiro(const Tabuleiro *tab, const Jogador *jogador)
{
    if (!tab || !tab->cabeca) return;

    /* Fundo da área central */
    DrawRectangle(BX + CS, BY + CS, 5*SW, 5*SH, (Color){8, 18, 38, 255});
    DrawText("REVITALIZA", BX+CS+10,  BY+CS+80,  28, (Color){255,140,30,60});
    DrawText("RECIFE",     BX+CS+60,  BY+CS+115, 28, (Color){255,140,30,60});

    /* Casas */
    Casa *c = tab->cabeca;
    do {
        Rectangle r   = render_casa_rect(c->id);
        Color     cor = cor_da_casa(c);

        DrawRectangleRec(r, (Color){10, 25, 55, 255});     /* fundo escuro */
        DrawRectangleRec(r, cor);                           /* camada de cor */

        /* borda: branca se tem dono, cinza se livre */
        Color borda = (c->proprietario >= 0)
                      ? (Color){255, 255, 255, 255}
                      : COR_BORDA;
        DrawRectangleLinesEx(r, 2, borda);

        /* ID no canto superior esquerdo */
        char id_str[4];
        snprintf(id_str, sizeof(id_str), "%02d", c->id);
        DrawText(id_str, (int)r.x + 4, (int)r.y + 4, 9, (Color){255,255,255,160});

        /* Nome da casa */
        desenhar_nome(c->nome, r, 8);

    } while ((c = c->next) != tab->cabeca);

    /* Peão do jogador */
    if (jogador && jogador->posicao) {
        Rectangle r  = render_casa_rect(jogador->posicao->id);
        float     cx = r.x + r.width  / 2.0f;
        float     cy = r.y + r.height / 2.0f - 8;

        DrawCircle((int)cx + 2, (int)cy + 2, 14, (Color){0, 0, 0, 120}); /* sombra */
        DrawCircle((int)cx,     (int)cy,     14, WHITE);
        DrawCircleLines((int)cx, (int)cy,    14, (Color){30,30,60,255});

        char ini[2] = { jogador->nome[0], '\0' };
        int  iw     = MeasureText(ini, 14);
        DrawText(ini, (int)(cx - iw/2), (int)(cy - 7), 14, (Color){10,20,50,255});
    }
}

/* ------------------------------------------------------------------ */
/* Render: HUD lateral                                                  */
/* ------------------------------------------------------------------ */

static void barra_progresso(int x, int y, int w, int h,
                             float ratio, Color cor)
{
    if (ratio > 1.0f) ratio = 1.0f;
    DrawRectangle(x, y, w, h, (Color){20, 30, 55, 255});
    DrawRectangle(x, y, (int)(w * ratio), h, cor);
    DrawRectangleLines(x, y, w, h, (Color){60, 80, 120, 255});
}

void render_hud(const Jogador *jogador, int ultimo_dado)
{
    const int HX  = 962;
    const int HW  = 308;

    /* fundo */
    DrawRectangle(960, 0, 320, 720, (Color){8, 18, 38, 255});
    DrawLineEx((Vector2){960, 0}, (Vector2){960, 720}, 2,
               (Color){255, 140, 30, 200});

    if (!jogador) return;

    int x = HX, y = 20;
    char buf[64];

    /* --- Título --- */
    DrawText("STATUS", x, y, 20, (Color){255, 140, 30, 255});
    y += 28;
    DrawLine(x, y, x + HW, y, (Color){255, 140, 30, 80});
    y += 14;

    /* --- Nome --- */
    DrawText(jogador->nome, x, y, 18, (Color){220, 235, 255, 255});
    y += 36;

    /* --- Moedas --- */
    DrawText("MOEDAS", x, y, 11, (Color){160, 160, 160, 255});
    y += 16;
    snprintf(buf, sizeof(buf), "$ %d", jogador->moedas);
    DrawText(buf, x, y, 24, (Color){255, 215, 0, 255});
    y += 42;

    DrawLine(x, y, x + HW, y, (Color){255, 140, 30, 50});
    y += 14;

    /* --- Pontos de impacto --- */
    DrawText("PONTOS DE IMPACTO", x, y, 11, (Color){160, 160, 160, 255});
    y += 20;

    /* Tecnologia */
    DrawText("TECNOLOGIA", x, y, 13, COR_TEC);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_TECNOLOGIA], META_PONTOS);
    DrawText(buf, x + HW - MeasureText(buf, 13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_TECNOLOGIA] / META_PONTOS, COR_TEC);
    y += 22;

    /* Turismo */
    DrawText("TURISMO", x, y, 13, COR_TUR);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_TURISMO], META_PONTOS);
    DrawText(buf, x + HW - MeasureText(buf, 13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_TURISMO] / META_PONTOS, COR_TUR);
    y += 22;

    /* Comércio */
    DrawText("COMERCIO", x, y, 13, COR_COM);
    snprintf(buf, sizeof(buf), "%d/%d", jogador->pontos[SETOR_COMERCIO], META_PONTOS);
    DrawText(buf, x + HW - MeasureText(buf, 13), y, 13, WHITE);
    y += 18;
    barra_progresso(x, y, HW, 10,
                    (float)jogador->pontos[SETOR_COMERCIO] / META_PONTOS, COR_COM);
    y += 32;

    DrawLine(x, y, x + HW, y, (Color){255, 140, 30, 50});
    y += 14;

    /* --- Posição atual --- */
    DrawText("POSICAO ATUAL", x, y, 11, (Color){160, 160, 160, 255});
    y += 18;
    if (jogador->posicao) {
        DrawText(jogador->posicao->nome, x, y, 13, (Color){220, 235, 255, 255});
        y += 20;

        Color sc = (Color){160, 160, 160, 255};
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

    DrawLine(x, y, x + HW, y, (Color){255, 140, 30, 50});
    y += 14;

    /* --- Último dado --- */
    DrawText("ULTIMO DADO", x, y, 11, (Color){160, 160, 160, 255});
    y += 18;
    if (ultimo_dado > 0) {
        snprintf(buf, sizeof(buf), "%d", ultimo_dado);
        DrawText(buf, x, y, 32, (Color){255, 140, 30, 255});
    } else {
        DrawText("-", x, y, 32, (Color){100, 100, 100, 255});
    }
    y += 56;

    DrawLine(x, y, x + HW, y, (Color){255, 140, 30, 50});
    y += 14;

    /* --- Controles --- */
    DrawText("CONTROLES", x, y, 11, (Color){160, 160, 160, 255});
    y += 18;
    DrawText("[ESPACO]  Rolar dado", x, y, 12, (Color){200, 220, 255, 180});
}
