#ifndef STATE_H
#define STATE_H

#include <SDL2/SDL.h>
#include "game.h"

typedef struct State {
    void (*init)(void *ctx);
    void (*handle_event)(void *ctx, SDL_Event *event);
    void (*update)(void *ctx, float dt);
    void (*render)(void *ctx);
    void (*cleanup)(void *ctx);
    void *ctx;
} State;

State* create_open_world_state(Game *game);

#endif