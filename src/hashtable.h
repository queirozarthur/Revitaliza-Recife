#ifndef HASHTABLE_H
#define HASHTABLE_H

#define HT_CAPACIDADE 31   /* primo > total de entradas esperadas (~37) */

/* Nó da lista encadeada dentro de cada bucket */
typedef struct HtNo {
    int          chave;
    void        *valor;     /* genérico: Carta* ou Casa* */
    struct HtNo *prox;
} HtNo;

typedef struct {
    HtNo **buckets;
    int    capacidade;
    int    tamanho;
} HashTable;

/* --- ciclo de vida --- */
HashTable *ht_criar(int capacidade);
void       ht_destruir(HashTable *ht, void (*liberar_valor)(void *));

/* --- operações --- */
int   ht_inserir(HashTable *ht, int chave, void *valor);
void *ht_buscar(const HashTable *ht, int chave);
int   ht_remover(HashTable *ht, int chave, void (*liberar_valor)(void *));

/* --- debug --- */
void  ht_imprimir(const HashTable *ht);

#endif /* HASHTABLE_H */
