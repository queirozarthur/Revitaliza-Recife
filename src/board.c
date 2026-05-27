#include "board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cJSON.h"

static TipoCasa str_para_tipo(const char *str) {
    if (strcmp(str, "CASA_INICIO") == 0) return CASA_INICIO;
    if (strcmp(str, "CASA_PROPRIEDADE") == 0) return CASA_PROPRIEDADE;
    if (strcmp(str, "CASA_EVENTO") == 0) return CASA_EVENTO;
    if (strcmp(str, "CASA_SORTE") == 0) return CASA_SORTE;
    if (strcmp(str, "CASA_AZAR") == 0) return CASA_AZAR;
    return CASA_INICIO;
}

static SetorCasa str_para_setor(const char *str) {
    if (strcmp(str, "SETOR_TURISMO") == 0) return SETOR_TURISMO;
    if (strcmp(str, "SETOR_COMERCIO") == 0) return SETOR_COMERCIO;
    if (strcmp(str, "SETOR_TECNOLOGIA") == 0) return SETOR_TECNOLOGIA;
    return SETOR_NEUTRO;
}

static Casa *casa_alocar(int id, const char *nome, TipoCasa tipo, SetorCasa setor, int custo, int pontos)
{
    Casa *c = (Casa *)calloc(1, sizeof(Casa));
    if (!c) return NULL;

    c->id            = id;
    c->tipo          = tipo;
    c->setor         = setor;
    c->custo         = custo;
    c->pontos        = pontos;
    c->proprietario  = -1;
    strncpy(c->nome, nome, MAX_NOME_CASA - 1);
    return c;
}

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

Tabuleiro *tabuleiro_criar(void)
{
    Tabuleiro *tab = (Tabuleiro *)calloc(1, sizeof(Tabuleiro));
    if (!tab) return NULL;

    FILE *f = fopen("assets/tabuleiro.json", "rb");
    if (!f) {
        printf("Erro ao abrir assets/tabuleiro.json\n");
        free(tab);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *json_data = malloc(fsize + 1);
    fread(json_data, 1, fsize, f);
    fclose(f);
    json_data[fsize] = 0;

    cJSON *json = cJSON_Parse(json_data);
    free(json_data);

    if (!json) {
        printf("Erro ao fazer parse do JSON do tabuleiro\n");
        free(tab);
        return NULL;
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        cJSON *id_json = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON *nome_json = cJSON_GetObjectItemCaseSensitive(item, "nome");
        cJSON *tipo_json = cJSON_GetObjectItemCaseSensitive(item, "tipo");
        cJSON *setor_json = cJSON_GetObjectItemCaseSensitive(item, "setor");
        cJSON *custo_json = cJSON_GetObjectItemCaseSensitive(item, "custo");
        cJSON *pontos_json = cJSON_GetObjectItemCaseSensitive(item, "pontos");

        if (cJSON_IsNumber(id_json) && cJSON_IsString(nome_json) && cJSON_IsString(tipo_json) &&
            cJSON_IsString(setor_json) && cJSON_IsNumber(custo_json) && cJSON_IsNumber(pontos_json)) {
            
            Casa *c = casa_alocar(id_json->valueint, nome_json->valuestring, 
                                  str_para_tipo(tipo_json->valuestring), 
                                  str_para_setor(setor_json->valuestring), 
                                  custo_json->valueint, pontos_json->valueint);
            if (c) {
                lista_inserir(tab, c);
            }
        }
    }

    cJSON_Delete(json);
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

    int ok = (tab->cabeca->prev->next == tab->cabeca);
    printf("Integridade circular: %s\n", ok ? "OK" : "ERRO");
}
