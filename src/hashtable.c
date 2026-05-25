#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Função de hash — divisão simples; eficiente para IDs inteiros       */
/* ------------------------------------------------------------------ */
static int hash(int chave, int capacidade)
{
    /* Garante índice positivo mesmo para chaves negativas */
    return ((chave % capacidade) + capacidade) % capacidade;
}

/* ------------------------------------------------------------------ */
/* Ciclo de vida                                                        */
/* ------------------------------------------------------------------ */

HashTable *ht_criar(int capacidade)
{
    HashTable *ht = (HashTable *)calloc(1, sizeof(HashTable));
    if (!ht) return NULL;

    ht->buckets = (HtNo **)calloc(capacidade, sizeof(HtNo *));
    if (!ht->buckets) { free(ht); return NULL; }

    ht->capacidade = capacidade;
    ht->tamanho    = 0;
    return ht;
}

void ht_destruir(HashTable *ht, void (*liberar_valor)(void *))
{
    if (!ht) return;

    for (int i = 0; i < ht->capacidade; i++) {
        HtNo *no = ht->buckets[i];
        while (no) {
            HtNo *prox = no->prox;
            if (liberar_valor) liberar_valor(no->valor);
            free(no);
            no = prox;
        }
    }
    free(ht->buckets);
    free(ht);
}

/* ------------------------------------------------------------------ */
/* Operações                                                            */
/* ------------------------------------------------------------------ */

int ht_inserir(HashTable *ht, int chave, void *valor)
{
    if (!ht) return 0;

    int idx = hash(chave, ht->capacidade);

    /* Atualiza se a chave já existe */
    for (HtNo *no = ht->buckets[idx]; no; no = no->prox) {
        if (no->chave == chave) {
            no->valor = valor;
            return 1;
        }
    }

    /* Insere na frente do bucket */
    HtNo *novo = (HtNo *)malloc(sizeof(HtNo));
    if (!novo) return 0;

    novo->chave        = chave;
    novo->valor        = valor;
    novo->prox         = ht->buckets[idx];
    ht->buckets[idx]   = novo;
    ht->tamanho++;
    return 1;
}

void *ht_buscar(const HashTable *ht, int chave)
{
    if (!ht) return NULL;

    int idx = hash(chave, ht->capacidade);
    for (HtNo *no = ht->buckets[idx]; no; no = no->prox) {
        if (no->chave == chave) return no->valor;
    }
    return NULL;
}

int ht_remover(HashTable *ht, int chave, void (*liberar_valor)(void *))
{
    if (!ht) return 0;

    int   idx  = hash(chave, ht->capacidade);
    HtNo *ant  = NULL;
    HtNo *no   = ht->buckets[idx];

    while (no) {
        if (no->chave == chave) {
            if (ant) ant->prox        = no->prox;
            else     ht->buckets[idx] = no->prox;

            if (liberar_valor) liberar_valor(no->valor);
            free(no);
            ht->tamanho--;
            return 1;
        }
        ant = no;
        no  = no->prox;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Debug                                                                */
/* ------------------------------------------------------------------ */

void ht_imprimir(const HashTable *ht)
{
    if (!ht) return;
    printf("=== HASH TABLE (cap=%d, tam=%d) ===\n", ht->capacidade, ht->tamanho);
    for (int i = 0; i < ht->capacidade; i++) {
        if (!ht->buckets[i]) continue;
        printf("  [%02d]", i);
        for (HtNo *no = ht->buckets[i]; no; no = no->prox)
            printf(" -> chave:%d", no->chave);
        printf("\n");
    }
}
