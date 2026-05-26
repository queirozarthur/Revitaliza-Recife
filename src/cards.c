#include "cards.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Dados estáticos das cartas                                           */
/* ------------------------------------------------------------------ */

static const Carta SORTE_DADOS[] = {
    { 100, "Patrocínio Cultural",
      "Uma empresa patrocina eventos culturais no Recife Antigo.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_TURISMO,    2 },

    { 101, "Hackathon no Porto",
      "Hackathon no Porto Digital gera soluções inovadoras.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_TECNOLOGIA, 2 },

    { 102, "Feira de Negócios",
      "Feira movimenta o comércio local do Bairro do Recife.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_COMERCIO,   2 },

    { 103, "Investidor Anjo",
      "Um investidor acredita no seu projeto. Ganhe 80 moedas.",
      CARTA_SORTE, EFEITO_GANHAR_MOEDAS, SETOR_NEUTRO,    80 },

    { 104, "Movimento Express",
      "Você encontra um atalho pelo Bairro do Recife. Avance 3 casas.",
      CARTA_SORTE, EFEITO_AVANCAR,       SETOR_NEUTRO,     3 },
};

static const Carta AZAR_DADOS[] = {
    { 200, "Obra na Via",
      "Obras interditam o acesso ao seu projeto. Perde 1 turno.",
      CARTA_AZAR, EFEITO_PERDER_TURNO,   SETOR_NEUTRO,     1 },

    { 201, "Embargo Fiscal",
      "A Receita cobra impostos atrasados. Pague 50 moedas.",
      CARTA_AZAR, EFEITO_PERDER_MOEDAS,  SETOR_NEUTRO,    50 },

    { 202, "Ato Vandálico",
      "Vandalismos prejudicam a imagem do turismo local. Perde 1 pt Turismo.",
      CARTA_AZAR, EFEITO_PERDER_PONTOS,  SETOR_TURISMO,    1 },

    { 203, "Retorno ao Início",
      "Burocracia força você a recomeçar do Marco Zero.",
      CARTA_AZAR, EFEITO_VOLTAR_INICIO,  SETOR_NEUTRO,     0 },
};

static const Carta EVENTO_DADOS[] = {
    { 300, "Festival do Manguebeat",
      "O festival atrai turistas de todo o Brasil. Todos ganham +1 pt Turismo.",
      CARTA_EVENTO, EFEITO_TODOS_GANHAM_PONTOS, SETOR_TURISMO,    1 },

    { 301, "Summit de Inovação",
      "Conferência de tecnologia impulsiona o Porto Digital. Todos ganham +1 pt Tecnologia.",
      CARTA_EVENTO, EFEITO_TODOS_GANHAM_PONTOS, SETOR_TECNOLOGIA, 1 },

    { 302, "Crise Econômica",
      "A crise afeta todos os empreendedores. Todos pagam 40 moedas.",
      CARTA_EVENTO, EFEITO_TODOS_PERDEM_MOEDAS, SETOR_NEUTRO,    40 },

    { 303, "Parceria Porto Digital",
      "Startups do Porto Digital recebem impulso. Donos de Tecnologia ganham +1 pt.",
      CARTA_EVENTO, EFEITO_PROPRIETARIOS_BONUS, SETOR_TECNOLOGIA, 1 },
};

/* ------------------------------------------------------------------ */
/* Fisher-Yates shuffle                                                 */
/* ------------------------------------------------------------------ */
static void embaralhar(int *ids, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j   = rand() % (i + 1);
        int tmp = ids[i];
        ids[i]  = ids[j];
        ids[j]  = tmp;
    }
}

/* ------------------------------------------------------------------ */
/* Inicializa um baralho: insere cartas na hash e embaralha os IDs     */
/* ------------------------------------------------------------------ */
static int init_baralho(HashTable *ht, Baralho *b,
                        const Carta *dados, int n)
{
    b->total = n;
    b->topo  = 0;
    for (int i = 0; i < n; i++) {
        Carta *c = (Carta *)malloc(sizeof(Carta));
        if (!c) return 0;
        *c = dados[i];
        if (!ht_inserir(ht, c->id, c)) { free(c); return 0; }
        b->ids[i] = c->id;
    }
    embaralhar(b->ids, n);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Ciclo de vida                                                        */
/* ------------------------------------------------------------------ */

SistemaCartas *cartas_criar(void)
{
#if MODO_APRESENTACAO
    srand(42);
#else
    srand((unsigned)time(NULL));
#endif

    SistemaCartas *sc = (SistemaCartas *)calloc(1, sizeof(SistemaCartas));
    if (!sc) return NULL;

    sc->tabela = ht_criar(HT_CAPACIDADE);
    if (!sc->tabela) { free(sc); return NULL; }

    int n_sorte  = (int)(sizeof(SORTE_DADOS)  / sizeof(SORTE_DADOS[0]));
    int n_azar   = (int)(sizeof(AZAR_DADOS)   / sizeof(AZAR_DADOS[0]));
    int n_evento = (int)(sizeof(EVENTO_DADOS) / sizeof(EVENTO_DADOS[0]));

    if (!init_baralho(sc->tabela, &sc->sorte,  SORTE_DADOS,  n_sorte)  ||
        !init_baralho(sc->tabela, &sc->azar,   AZAR_DADOS,   n_azar)   ||
        !init_baralho(sc->tabela, &sc->evento, EVENTO_DADOS, n_evento)) {
        cartas_destruir(sc);
        return NULL;
    }

    return sc;
}

void cartas_destruir(SistemaCartas *sc)
{
    if (!sc) return;
    /* free libera cada Carta* alocado em init_baralho */
    ht_destruir(sc->tabela, free);
    free(sc);
}

/* ------------------------------------------------------------------ */
/* Operações de jogo                                                    */
/* ------------------------------------------------------------------ */

/* Puxar do topo e avançar; ao esgotar, recomeça do início (wrap) */
const Carta *cartas_puxar_sorte(SistemaCartas *sc)
{
    if (!sc || sc->sorte.total == 0) return NULL;
    int id = sc->sorte.ids[sc->sorte.topo];
    sc->sorte.topo = (sc->sorte.topo + 1) % sc->sorte.total;
    return (const Carta *)ht_buscar(sc->tabela, id);
}

const Carta *cartas_puxar_azar(SistemaCartas *sc)
{
    if (!sc || sc->azar.total == 0) return NULL;
    int id = sc->azar.ids[sc->azar.topo];
    sc->azar.topo = (sc->azar.topo + 1) % sc->azar.total;
    return (const Carta *)ht_buscar(sc->tabela, id);
}

const Carta *cartas_puxar_evento(SistemaCartas *sc)
{
    if (!sc || sc->evento.total == 0) return NULL;
    int id = sc->evento.ids[sc->evento.topo];
    sc->evento.topo = (sc->evento.topo + 1) % sc->evento.total;
    return (const Carta *)ht_buscar(sc->tabela, id);
}

const Carta *cartas_buscar(const SistemaCartas *sc, int id)
{
    if (!sc) return NULL;
    return (const Carta *)ht_buscar(sc->tabela, id);
}

/* ------------------------------------------------------------------ */
/* Aplicação de efeito                                                  */
/* ------------------------------------------------------------------ */
#include "player.h"

Casa *carta_aplicar_efeito(const Carta *carta,
                           Jogador *jogadores, int num_jogadores,
                           int idx_atual,
                           const Tabuleiro *tab)
{
    if (!carta || !jogadores) return NULL;

    Jogador *j = &jogadores[idx_atual];

    switch (carta->efeito) {

        case EFEITO_GANHAR_PONTOS:
            j->pontos[carta->setor] += carta->valor;
            break;

        case EFEITO_PERDER_PONTOS:
            j->pontos[carta->setor] -= carta->valor;
            if (j->pontos[carta->setor] < 0)
                j->pontos[carta->setor] = 0;
            break;

        case EFEITO_GANHAR_MOEDAS:
            j->moedas += carta->valor;
            break;

        case EFEITO_PERDER_MOEDAS:
            j->moedas -= carta->valor;
            if (j->moedas < 0) j->moedas = 0;
            break;

        case EFEITO_AVANCAR:
            j->posicao = tabuleiro_mover(j->posicao, carta->valor);
            return j->posicao;

        case EFEITO_VOLTAR_INICIO:
            j->posicao = tabuleiro_buscar_id(tab, 0);
            return j->posicao;

        case EFEITO_PERDER_TURNO:
            j->turnos_bloqueado += carta->valor;
            break;

        case EFEITO_TODOS_GANHAM_PONTOS:
            for (int i = 0; i < num_jogadores; i++)
                jogadores[i].pontos[carta->setor] += carta->valor;
            break;

        case EFEITO_TODOS_PERDEM_MOEDAS:
            for (int i = 0; i < num_jogadores; i++) {
                jogadores[i].moedas -= carta->valor;
                if (jogadores[i].moedas < 0) jogadores[i].moedas = 0;
            }
            break;

        case EFEITO_PROPRIETARIOS_BONUS:
            for (int i = 0; i < num_jogadores; i++) {
                Casa *c = tab->cabeca;
                do {
                    if (c->setor == carta->setor && c->proprietario == i) {
                        jogadores[i].pontos[carta->setor] += carta->valor;
                        break;
                    }
                    c = c->next;
                } while (c != tab->cabeca);
            }
            break;
    }

    return NULL;
}