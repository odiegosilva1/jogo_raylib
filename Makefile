RAYLIB_PATH = /tmp/raylib
CC = gcc
CFLAGS = -Wall -I$(RAYLIB_PATH)/src
LDFLAGS = -L$(RAYLIB_PATH)/build/raylib $(RAYLIB_PATH)/build/raylib/external/glfw/src/libglfw3.a
LIBS = -l:libraylib.so -lm -lrt -lpthread -ldl -lX11 -lXrandr -lXinerama -lXcursor -lGL
EXEC = jogo
SRC = main.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS) $(LIBS) -Wl,-rpath,$(RAYLIB_PATH)/build/raylib

run: $(EXEC)
	LD_LIBRARY_PATH=$(RAYLIB_PATH)/build/raylib ./$(EXEC)

clean:
	rm -f $(EXEC)

.PHONY: all run clean