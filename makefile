CC = gcc

SRC_DIR = .
INCLUDE_DIR = src/include
LIB_DIR = src/lib

ifeq ($(OS),Windows_NT)
  EXE_EXT := .exe
else
  EXE_EXT :=
endif

PKG_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer 2>/dev/null)
PKG_LIBS := $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer 2>/dev/null)

ifeq ($(OS),Windows_NT)
  CFLAGS = -I$(INCLUDE_DIR) -Wall -Wextra
  LDFLAGS = -L$(LIB_DIR) -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -mwindows
else
  CFLAGS = -I$(INCLUDE_DIR) -Wall -Wextra $(PKG_CFLAGS)
  LDFLAGS = $(if $(PKG_LIBS),$(PKG_LIBS),-lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer)
endif

TARGET = c_tale$(EXE_EXT)
SRC = main.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	cp "$(LIB_DIR)/SDL2.dll" . 2>/dev/null || true
	cp "$(LIB_DIR)/SDL2_image.dll" . 2>/dev/null || true
	cp "$(LIB_DIR)/SDL2_ttf.dll" . 2>/dev/null || true
	cp "$(LIB_DIR)/SDL2_mixer.dll" . 2>/dev/null || true
	./$(TARGET)

.PHONY: all clean run