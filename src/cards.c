#include "cards.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const Carta SORTE_DADOS[] = {
    { 100, "Patrocínio Cultural",
      "Uma empresa patrocina eventos culturais no Recife Antigo.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_TURISMO,    3 },

    { 101, "Hackathon no Porto",
      "Hackathon no Porto Digital gera soluções inovadoras.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_TECNOLOGIA, 3 },

    { 102, "Feira de Negócios",
      "Feira movimenta o comércio local do Bairro do Recife.",
      CARTA_SORTE, EFEITO_GANHAR_PONTOS, SETOR_COMERCIO,   3 },

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

static void embaralhar(int *ids, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j   = rand() % (i + 1);
        int tmp = ids[i];
        ids[i]  = ids[j];
        ids[j]  = tmp;
    }
}

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
    
    ht_destruir(sc->tabela, free);
    free(sc);
}

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
            j->pontos[carta->setor] += carta->valor * (j->turnos_festa > 0 ? 2 : 1);
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
            for (int i = 0; i < num_jogadores; i++) {
                int multi = jogadores[i].turnos_festa > 0 ? 2 : 1;
                jogadores[i].pontos[carta->setor] += carta->valor * multi;
            }
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
                        int multi = jogadores[i].turnos_festa > 0 ? 2 : 1;
                        jogadores[i].pontos[carta->setor] += carta->valor * multi;
                        break;
                    }
                    c = c->next;
                } while (c != tab->cabeca);
            }
            break;
    }

    return NULL;
}

void usar_carta_acao(Jogador *jogadores, int num_jogadores, int jogador_atual, int acao_id, int alvo_idx, Tabuleiro *tab) {
    Jogador *j = &jogadores[jogador_atual];
    int multi = (j->turnos_festa > 0 && acao_id != 1) ? 2 : 1;
    
    if (acao_id == 0) {
        
        j->pontos[SETOR_TURISMO] += 1 * multi;
        Casa *c = tab->cabeca;
        do {
            if (c->tipo == CASA_PROPRIEDADE && c->setor == SETOR_TURISMO && c->proprietario == jogador_atual) {
                j->pontos[SETOR_TURISMO] += 1 * multi;
            }
            c = c->next;
        } while (c != tab->cabeca);
    } else if (acao_id == 1) {
        
        j->turnos_festa = 2;
    } else if (acao_id == 2) {
        
        j->pontos[SETOR_COMERCIO] += 1 * multi;
        Casa *c = tab->cabeca;
        do {
            if (c->tipo == CASA_PROPRIEDADE && c->setor == SETOR_COMERCIO && c->proprietario == jogador_atual) {
                j->pontos[SETOR_COMERCIO] += 1 * multi;
            }
            c = c->next;
        } while (c != tab->cabeca);
    } else if (acao_id == 3) {
        
        if (alvo_idx >= 0 && alvo_idx < num_jogadores && alvo_idx != jogador_atual) {
            Jogador *alvo = &jogadores[alvo_idx];
            int a_multi = alvo->turnos_festa > 0 ? 2 : 1;
            
            j->pontos[SETOR_TECNOLOGIA] += 1 * multi;
            j->pontos[SETOR_TURISMO] += 1 * multi;
            j->pontos[SETOR_COMERCIO] += 1 * multi;
            alvo->pontos[SETOR_TECNOLOGIA] += 1 * a_multi;
            alvo->pontos[SETOR_TURISMO] += 1 * a_multi;
            alvo->pontos[SETOR_COMERCIO] += 1 * a_multi;
            
            int j_setores[4] = {0};
            int a_setores[4] = {0};
            Casa *c = tab->cabeca;
            do {
                if (c->tipo == CASA_PROPRIEDADE && c->proprietario == jogador_atual) j_setores[c->setor] = 1;
                if (c->tipo == CASA_PROPRIEDADE && c->proprietario == alvo_idx) a_setores[c->setor] = 1;
                c = c->next;
            } while (c != tab->cabeca);
            
            int diff_sectors = 0;
            for (int s=1; s<=3; s++) {
                if (j_setores[s]) {
                    for (int s2=1; s2<=3; s2++) {
                        if (a_setores[s2] && s != s2) { diff_sectors = 1; break; }
                    }
                }
            }
            
            if (diff_sectors) {
                j->pontos[SETOR_TECNOLOGIA] += 1 * multi;
                j->pontos[SETOR_TURISMO] += 1 * multi;
                j->pontos[SETOR_COMERCIO] += 1 * multi;
                alvo->pontos[SETOR_TECNOLOGIA] += 1 * a_multi;
                alvo->pontos[SETOR_TURISMO] += 1 * a_multi;
                alvo->pontos[SETOR_COMERCIO] += 1 * a_multi;
            }
        }
    }
}