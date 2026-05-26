#include "player.h"
#include <string.h>
#include "raylib.h"

Jogador jogador_criar(const char *nome, Casa *inicio, TipoJogador tipo, int avatar_id)
{
    Jogador j = {0};
    strncpy(j.nome, nome, MAX_NOME_JOGADOR - 1);
    j.moedas  = MOEDAS_INICIAIS;
    j.posicao = inicio;
    j.tipo    = tipo;
    j.avatar_id = avatar_id;
    j.turnos_festa = 0;
    
    // Distribui 2 cartas aleatórias. Carta 2 (index 1) é mais rara (10% de chance).
    // As outras 3 têm 30% de chance cada.
    for (int i = 0; i < 2; i++) {
        int rnd = GetRandomValue(1, 100);
        if (rnd <= 10) j.cartas_acao[i] = 1; // Festa (rara)
        else if (rnd <= 40) j.cartas_acao[i] = 0; // Turismo
        else if (rnd <= 70) j.cartas_acao[i] = 2; // Comercio
        else j.cartas_acao[i] = 3; // Parceria
    }
    
    return j;
}

int jogador_venceu(const Jogador *j)
{
    return j->pontos[SETOR_TECNOLOGIA] >= META_PONTOS &&
           j->pontos[SETOR_TURISMO]    >= META_PONTOS &&
           j->pontos[SETOR_COMERCIO]   >= META_PONTOS;
}
