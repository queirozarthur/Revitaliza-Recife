#ifndef CARDS_H
#define CARDS_H

#include "hashtable.h"
#include "board.h"

/* ------------------------------------------------------------------ */
/* Modo de apresentação                                                 */
/*   1 → srand(42)          — ordem fixa e previsível na demo          */
/*   0 → srand(time(NULL))  — ordem aleatória real                     */
/* ------------------------------------------------------------------ */
#define MODO_APRESENTACAO 1

#define MAX_TITULO        64
#define MAX_DESCRICAO    128
#define MAX_CARTAS_TIPO    8   /* limite por categoria */

/* IDs por faixa: Sorte 100–109 | Azar 200–209 | Evento 300–309 */

typedef enum { CARTA_SORTE, CARTA_AZAR, CARTA_EVENTO } TipoCarta;

typedef enum {
    EFEITO_GANHAR_PONTOS,       /* jogador atual ganha `valor` pts em `setor`      */
    EFEITO_PERDER_PONTOS,       /* jogador atual perde `valor` pts em `setor`      */
    EFEITO_GANHAR_MOEDAS,       /* jogador atual ganha `valor` moedas              */
    EFEITO_PERDER_MOEDAS,       /* jogador atual paga  `valor` moedas              */
    EFEITO_AVANCAR,             /* jogador atual avança `valor` casas              */
    EFEITO_VOLTAR_INICIO,       /* jogador vai para o Marco Zero (casa 0)          */
    EFEITO_PERDER_TURNO,        /* jogador perde `valor` turno(s)                  */
    EFEITO_TODOS_GANHAM_PONTOS, /* todos ganham `valor` pts em `setor`             */
    EFEITO_TODOS_PERDEM_MOEDAS, /* todos pagam `valor` moedas                      */
    EFEITO_PROPRIETARIOS_BONUS  /* donos de propriedades de `setor` ganham `valor` */
} TipoEfeito;

typedef struct {
    int        id;
    char       titulo[MAX_TITULO];
    char       descricao[MAX_DESCRICAO];
    TipoCarta  tipo;
    TipoEfeito efeito;
    SetorCasa  setor;   /* setor afetado; SETOR_NEUTRO se não aplicável */
    int        valor;   /* quantidade de pontos, moedas ou casas        */
} Carta;

/* Baralho de um tipo de carta: IDs embaralhados + ponteiro de topo */
typedef struct {
    int ids[MAX_CARTAS_TIPO];
    int topo;
    int total;
} Baralho;

/* Contêiner principal — uma tabela hash + três baralhos */
typedef struct {
    HashTable *tabela;
    Baralho    sorte;
    Baralho    azar;
    Baralho    evento;
} SistemaCartas;

/* --- ciclo de vida --- */
SistemaCartas *cartas_criar(void);
void           cartas_destruir(SistemaCartas *sc);

/* --- operações de jogo --- */
const Carta *cartas_puxar_sorte(SistemaCartas *sc);
const Carta *cartas_puxar_azar(SistemaCartas *sc);
const Carta *cartas_puxar_evento(SistemaCartas *sc);
const Carta *cartas_buscar(const SistemaCartas *sc, int id);

#endif /* CARDS_H */
