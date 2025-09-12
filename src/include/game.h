#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum {
    STATE_TITLE,
    STATE_OPEN_WORLD,
    STATE_BATTLE,
    STATE_EXIT
} GameStateID;

typedef struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running;
    GameStateID current_state;
} Game;

bool sdl_initialize(Game *game);
void game_cleanup(Game *game);

SDL_Texture* createTexture(SDL_Renderer *renderer, const char *file);
void object_cleanup(SDL_Texture *texture);

#endif