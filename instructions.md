Analise e avalie se o projeto atual está alinhado com a descrição abaixo.

Projeto: Jogo de tabuleiro em C com Raylib (1280×720), estilo Banco Imobiliário, temática de revitalização do Recife Antigo / Porto Digital. Desenvolvido por Cauã, Bernardo, João Arthur e Arthur Reis como trabalho de AED.

Estrutura de Dados
Tabuleiro — Lista Duplamente Encadeada Circular com 24 casas (src/board.h / board.c):

Cantos nos IDs 0 (Marco Zero), 6, 12, 18
Tipos de casa: CASA_INICIO, CASA_PROPRIEDADE, CASA_SORTE, CASA_AZAR, CASA_EVENTO
Setores: SETOR_TECNOLOGIA (azul), SETOR_TURISMO (verde), SETOR_COMERCIO (laranja)
Cada Casa tem: id, nome, tipo, setor, custo, pontos, proprietario (inicializado em -1)
Cartas — Hash Table com encadeamento separado, capacidade 31 (src/hashtable.h / cards.h / cards.c):

IDs: Sorte 100–104, Azar 200–203, Evento 300–303
10 tipos de efeito: ganhar/perder pontos, ganhar/perder moedas, avançar, voltar ao início, perder turno, efeitos coletivos
MODO_APRESENTACAO 1 usa srand(42) para ordem fixa
carta_aplicar_efeito() já implementada e integrada
Jogador — Struct simples (src/player.h / player.c):

nome, moedas (300 iniciais), pontos[4] (por setor), posicao, turnos_bloqueado
Vitória: 20 pontos em cada um dos 3 setores
Atualmente só existe 1 jogador singular — sem array, sem bots
Interface Gráfica
Máquina de estados principal (src/main.c):


TELA_MENU  →  TELA_JOGO  →  TELA_RESULTADO
Máquina de estados do turno (src/render.h — AnimacaoTurno):


TURNO_AGUARDANDO
  → TURNO_DADO_GIRANDO      (1.8s de animação de shake)
  → TURNO_PINO_MOVENDO      (lerp suavizado + arco, 0.28s/passo)
  → TURNO_MOSTRANDO_CARTA   (overlay com [ESPAÇO] para fechar e aplicar)
  → TURNO_MOSTRANDO_PROPRIEDADE  (painel de compra/aluguel)
Renderização (src/render.c):

render_tabuleiro() — desenha as 24 casas com cores por setor + pino animado
render_hud() — painel lateral direito com moedas, barras de progresso por setor, posição atual
render_dado() — dado com pontos reais, animação de shake em 3 fases (rápido→médio→lento)
render_carta_overlay() — painel 500×380 centralizado, cabeçalho colorido por tipo, word-wrap na descrição
render_propriedade_overlay() — painel 480×360 com botões [C] Comprar / [X] Passar (ou [ESPAÇO] Pagar aluguel), botão [C] esmaecido se sem moedas
Geometria do tabuleiro: BX=20, BY=20, CS=110, SW=140, SH=88 — cantos quadrados, laterais retangulares

O que foi compilado e funciona
Menu → Jogo → tabuleiro completo renderizado
Dado animado, pino andando casa por casa
Cartas sorte/azar/evento com overlay e efeitos aplicados
Compra e aluguel de propriedades com painel de decisão
HUD com progresso por setor
Build: gcc -Wall -Wextra -std=c11 sem nenhum warning

O que ainda não existe (próximos passos pedidos)
is_bot / TipoJogador na struct Jogador
Array de 4 jogadores substituindo o Jogador jogador singular
TELA_SELECAO — nova tela entre menu e jogo perguntando quantos humanos jogam (1–4), preenchendo o restante com bots
Controle de turno — variável jogador_atual indicando qual dos 4 está jogando
IA dos bots — rolar dado e tomar decisões automaticamente
Ranking com Insertion Sort (módulo ranking.h / ranking.c) — não iniciado
Makefile

CC     = C:/msys64/ucrt64/bin/gcc.exe
CFLAGS = -Wall -Wextra -std=c11 -Isrc -IC:/msys64/ucrt64/include
LDFLAGS = -LC:/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
SRC = src/main.c src/board.c src/hashtable.c src/cards.c src/player.c src/render.c
OUT = recife.exe