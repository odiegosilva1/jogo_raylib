RAYLIB_PATH = /tmp/raylib_build
CC = gcc
CFLAGS = -Wall -I$(RAYLIB_PATH)/src
LDFLAGS = -L$(RAYLIB_PATH)/build/raylib $(RAYLIB_PATH)/examples/others/external/lib/libglfw3.a $(RAYLIB_PATH)/build/raylib/libraylib.a -lm -lrt -lpthread -ldl -lX11 -lXrandr -lXinerama -lXcursor -lGL
LIBS =
EXEC = jogo
SRC = main.c

all: sprites $(EXEC)

sprites:
	python3 generate_sprites.py

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS) $(LIBS)

run: $(EXEC)
	cp -r resources $(shell dirname $(EXEC))/
	cp grass.png $(shell dirname $(EXEC))/
	LD_LIBRARY_PATH=$(RAYLIB_PATH)/build/raylib ./$(EXEC)

clean:
	rm -f $(EXEC)

.PHONY: all run clean sprites