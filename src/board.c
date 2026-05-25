#include "board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Dados estáticos das 24 casas temáticas                              */
/* ------------------------------------------------------------------ */
static const struct {
    const char *nome;
    TipoCasa    tipo;
    SetorCasa   setor;
    int         custo;
    int         pontos;
} CASAS[] = {
/*  0 */ { "Marco Zero",              CASA_INICIO,      SETOR_NEUTRO,       0,  0 },
/*  1 */ { "Rua do Bom Jesus",        CASA_PROPRIEDADE, SETOR_TURISMO,     60,  3 },
/*  2 */ { "Torre Malakoff",          CASA_PROPRIEDADE, SETOR_TURISMO,     80,  4 },
/*  3 */ { "Carta de Evento",         CASA_EVENTO,      SETOR_NEUTRO,       0,  0 },
/*  4 */ { "Paço Alfândega",          CASA_PROPRIEDADE, SETOR_COMERCIO,    90,  4 },
/*  5 */ { "Cais do Porto",           CASA_PROPRIEDADE, SETOR_COMERCIO,   100,  5 },
/*  6 */ { "Instituto R. Brennand",   CASA_PROPRIEDADE, SETOR_TURISMO,    110,  5 },
/*  7 */ { "Carta de Sorte",          CASA_SORTE,       SETOR_NEUTRO,       0,  0 },
/*  8 */ { "Paço do Frevo",           CASA_PROPRIEDADE, SETOR_TURISMO,    120,  5 },
/*  9 */ { "Porto Digital",           CASA_PROPRIEDADE, SETOR_TECNOLOGIA, 150,  7 },
/* 10 */ { "Hub de Inovação",         CASA_PROPRIEDADE, SETOR_TECNOLOGIA, 130,  6 },
/* 11 */ { "Carta de Evento",         CASA_EVENTO,      SETOR_NEUTRO,       0,  0 },
/* 12 */ { "Mercado de São José",     CASA_PROPRIEDADE, SETOR_COMERCIO,   110,  5 },
/* 13 */ { "Casa da Cultura",         CASA_PROPRIEDADE, SETOR_TURISMO,    100,  5 },
/* 14 */ { "Carta de Azar",           CASA_AZAR,        SETOR_NEUTRO,       0,  0 },
/* 15 */ { "Armazém da Criatividade", CASA_PROPRIEDADE, SETOR_TECNOLOGIA, 140,  6 },
/* 16 */ { "Museu Cais do Sertão",    CASA_PROPRIEDADE, SETOR_TURISMO,    130,  6 },
/* 17 */ { "Startup Recife",          CASA_PROPRIEDADE, SETOR_TECNOLOGIA, 160,  7 },
/* 18 */ { "Carta de Sorte",          CASA_SORTE,       SETOR_NEUTRO,       0,  0 },
/* 19 */ { "Praça do Arsenal",        CASA_PROPRIEDADE, SETOR_TURISMO,    120,  5 },
/* 20 */ { "Mercado RioMar",          CASA_PROPRIEDADE, SETOR_COMERCIO,   150,  7 },
/* 21 */ { "Centro de Convenções",    CASA_PROPRIEDADE, SETOR_COMERCIO,   130,  6 },
/* 22 */ { "Carta de Azar",           CASA_AZAR,        SETOR_NEUTRO,       0,  0 },
/* 23 */ { "Acelerador de Startups",  CASA_PROPRIEDADE, SETOR_TECNOLOGIA, 170,  8 },
};

#define NUM_CASAS (int)(sizeof(CASAS) / sizeof(CASAS[0]))

/* ------------------------------------------------------------------ */
/* Funções internas                                                     */
/* ------------------------------------------------------------------ */

static Casa *casa_alocar(int id)
{
    Casa *c = (Casa *)calloc(1, sizeof(Casa));
    if (!c) return NULL;

    c->id            = id;
    c->tipo          = CASAS[id].tipo;
    c->setor         = CASAS[id].setor;
    c->custo         = CASAS[id].custo;
    c->pontos        = CASAS[id].pontos;
    c->proprietario  = -1;
    strncpy(c->nome, CASAS[id].nome, MAX_NOME_CASA - 1);
    return c;
}

/* Insere `nova` ao final da lista circular (antes de cabeca) */
static void lista_inserir(Tabuleiro *tab, Casa *nova)
{
    if (tab->cabeca == NULL) {
        nova->next = nova;
        nova->prev = nova;
        tab->cabeca = nova;
    } else {
        Casa *ultimo   = tab->cabeca->prev;
        ultimo->next   = nova;
        nova->prev     = ultimo;
        nova->next     = tab->cabeca;
        tab->cabeca->prev = nova;
    }
    tab->tamanho++;
}

/* ------------------------------------------------------------------ */
/* API pública                                                          */
/* ------------------------------------------------------------------ */

Tabuleiro *tabuleiro_criar(void)
{
    Tabuleiro *tab = (Tabuleiro *)calloc(1, sizeof(Tabuleiro));
    if (!tab) return NULL;

    for (int i = 0; i < NUM_CASAS; i++) {
        Casa *c = casa_alocar(i);
        if (!c) {
            tabuleiro_destruir(tab);
            return NULL;
        }
        lista_inserir(tab, c);
    }
    return tab;
}

void tabuleiro_destruir(Tabuleiro *tab)
{
    if (!tab) return;

    if (tab->cabeca) {
        Casa *cur = tab->cabeca;
        Casa *prox;
        do {
            prox = cur->next;
            free(cur);
            cur = prox;
        } while (cur != tab->cabeca);
    }
    free(tab);
}

Casa *tabuleiro_mover(Casa *atual, int passos)
{
    if (!atual) return NULL;

    Casa *c = atual;
    if (passos >= 0) {
        for (int i = 0; i < passos; i++)
            c = c->next;
    } else {
        for (int i = 0; i > passos; i--)
            c = c->prev;
    }
    return c;
}

Casa *tabuleiro_buscar_id(const Tabuleiro *tab, int id)
{
    if (!tab || !tab->cabeca) return NULL;

    Casa *c = tab->cabeca;
    do {
        if (c->id == id) return c;
        c = c->next;
    } while (c != tab->cabeca);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Debug                                                                */
/* ------------------------------------------------------------------ */

static const char *str_tipo(TipoCasa t)
{
    switch (t) {
        case CASA_INICIO:      return "INICIO ";
        case CASA_PROPRIEDADE: return "PROP   ";
        case CASA_EVENTO:      return "EVENTO ";
        case CASA_SORTE:       return "SORTE  ";
        case CASA_AZAR:        return "AZAR   ";
        default:               return "?      ";
    }
}

static const char *str_setor(SetorCasa s)
{
    switch (s) {
        case SETOR_TECNOLOGIA: return "TEC";
        case SETOR_TURISMO:    return "TUR";
        case SETOR_COMERCIO:   return "COM";
        default:               return "---";
    }
}

void tabuleiro_imprimir(const Tabuleiro *tab)
{
    if (!tab || !tab->cabeca) return;

    printf("=== TABULEIRO (%d casas) ===\n", tab->tamanho);
    Casa *c = tab->cabeca;
    do {
        printf("[%02d] %-28s | %s | %s | custo:%3d | pts:%d\n",
               c->id, c->nome,
               str_tipo(c->tipo),
               str_setor(c->setor),
               c->custo, c->pontos);
        c = c->next;
    } while (c != tab->cabeca);

    /* Verifica integridade da lista circular */
    int ok = (tab->cabeca->prev->next == tab->cabeca);
    printf("Integridade circular: %s\n", ok ? "OK" : "ERRO");
}
