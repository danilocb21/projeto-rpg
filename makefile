# Compilador
CC = gcc

# Diretórios - mantendo sua estrutura atual
SRC_DIR = .
INCLUDE_DIR = src/include
LIB_DIR = src/lib

# Detecta SO e extensão do executável
ifeq ($(OS),Windows_NT)
  EXE_EXT := .exe
else
  EXE_EXT :=
endif

# pkg-config para SDL2 + SDL2_image (se disponível)
PKG_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs sdl2 SDL2_image 2>/dev/null)

# Flags (fall back para -lSDL2 -lSDL2_image se pkg-config não existir)
ifeq ($(OS),Windows_NT)
  CFLAGS  = -I$(INCLUDE_DIR) -Wall -Wextra
  LDFLAGS = -L$(LIB_DIR) -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mwindows
else
  CFLAGS  = -I$(INCLUDE_DIR) -Wall -Wextra $(PKG_CFLAGS)
  LDFLAGS = $(if $(PKG_LIBS),$(PKG_LIBS),-lSDL2 -lSDL2_image)
endif

# Nome do executável
TARGET = c_tale$(EXE_EXT)

# Arquivos fonte (ajuste aqui se tiver mais .c)
SRC = main.c
OBJ = $(SRC:.c=.o)

# Regra padrão
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar
clean:
	rm -f $(OBJ) $(TARGET)

# Regra para executar (tenta copiar as DLLs do LIB_DIR se existirem)
run: $(TARGET)
	cp "$(LIB_DIR)/SDL2.dll" . 2>/dev/null || true
	cp "$(LIB_DIR)/SDL2_image.dll" . 2>/dev/null || true
	./$(TARGET)

.PHONY: all clean run