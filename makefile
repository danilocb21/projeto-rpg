# Compilador
CC = gcc

# Diretórios - mantendo sua estrutura atual
SRC_DIR = .
INCLUDE_DIR = src/include
LIB_DIR = src/lib

# Flags de compilação
CFLAGS = -I$(INCLUDE_DIR) -Wall -Wextra

# Flags de linker (ordem correta para SDL2 com MinGW)
LDFLAGS = -L$(LIB_DIR) -lmingw32 -lSDL2main -lSDL2 -mwindows

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