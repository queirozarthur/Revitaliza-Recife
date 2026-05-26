CC     = C:/msys64/ucrt64/bin/gcc.exe
CFLAGS = -Wall -Wextra -std=c11 -Isrc -IC:/msys64/ucrt64/include
LDFLAGS = -LC:/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
SRC = src/main.c src/board.c src/hashtable.c src/cards.c src/player.c src/render.c src/ranking.c
OUT = recife.exe

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)

.PHONY: clean
