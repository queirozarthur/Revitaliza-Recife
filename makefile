
CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -Isrc -IC:/raylib/raylib/src
LDFLAGS = -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
 
SRC = src/main.c src/board.c src/hashtable.c src/cards.c src/player.c src/render.c
OUT = recife.exe
 
.PHONY: all clean
 
all: $(OUT)
 
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
 
clean:
	del /Q $(OUT) 2>NUL || rm -f $(OUT)
