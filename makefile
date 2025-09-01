# Compilador
CC = gcc

# Diretórios - mantendo sua estrutura atual
SRC_DIR = .
INCLUDE_DIR = src/include
LIB_DIR = src/lib

# Detecta SO
PKG_SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
PKG_SDL2_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null)

ifeq ($(OS),Windows_NT)
  CFLAGS  = -I$(INCLUDE_DIR) -Wall -Wextra
  LDFLAGS = -L$(LIB_DIR) -lmingw32 -lSDL2main -lSDL2 -mwindows
  RUN_DEPS = copy-dll
else
  CFLAGS  = -I$(INCLUDE_DIR) -Wall -Wextra $(PKG_SDL2_CFLAGS)
  LDFLAGS = $(if $(PKG_SDL2_LIBS),$(PKG_SDL2_LIBS),-lSDL2)
endif

# Nome do executável
TARGET = c_tale.exe

# Arquivo fonte principal
SRC = main.c

# Regra padrão
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Regra para limpar
clean:
	rm -f $(TARGET)

# Regra para executar (copia a DLL se necessário)
run: $(TARGET)
	cp SDL2.dll $(TARGET) 2>/dev/null || true
	./$(TARGET)

.PHONY: all clean run
PKG_SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
PKG_SDL2_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null)