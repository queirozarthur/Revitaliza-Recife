#ifndef HASHTABLE_H
#define HASHTABLE_H

#define HT_CAPACIDADE 31   

typedef struct HtNo {
    int          chave;
    void        *valor;     
    struct HtNo *prox;
} HtNo;

typedef struct {
    HtNo **buckets;
    int    capacidade;
    int    tamanho;
} HashTable;

HashTable *ht_criar(int capacidade);
void       ht_destruir(HashTable *ht, void (*liberar_valor)(void *));

int   ht_inserir(HashTable *ht, int chave, void *valor);
void *ht_buscar(const HashTable *ht, int chave);
int   ht_remover(HashTable *ht, int chave, void (*liberar_valor)(void *));

void  ht_imprimir(const HashTable *ht);

#endif 
