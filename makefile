CC     = C:/raylib/w64devkit/bin/gcc.exe -B C:/raylib/w64devkit/bin
CFLAGS = -Wall -Wextra -std=c11 -Isrc -IC:/raylib/raylib/src
LDFLAGS = -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
SRC = src/main.c src/board.c src/hashtable.c src/cards.c src/player.c src/render.c src/ranking.c src/cJSON.c
OUT = recife.exe

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)

.PHONY: clean
