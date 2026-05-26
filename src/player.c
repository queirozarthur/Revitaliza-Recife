#include "player.h"
#include <string.h>

Jogador jogador_criar(const char *nome, Casa *inicio, TipoJogador tipo)
{
    Jogador j = {0};
    strncpy(j.nome, nome, MAX_NOME_JOGADOR - 1);
    j.moedas  = MOEDAS_INICIAIS;
    j.posicao = inicio;
    j.tipo    = tipo;
    return j;
}

int jogador_venceu(const Jogador *j)
{
    return j->pontos[SETOR_TECNOLOGIA] >= META_PONTOS &&
           j->pontos[SETOR_TURISMO]    >= META_PONTOS &&
           j->pontos[SETOR_COMERCIO]   >= META_PONTOS;
}
