#include "include/game.h"
#include "include/state.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    Game game;
    if (sdl_initialize(&game)) {
        game_cleanup(&game);
        return 1;
    }

    State *state = create_open_world_state(&game);
    state->init(state->ctx);
    Uint32 last = SDL_GetTicks();
    while (game.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            state->handle_event(state->ctx, &event);
        }
        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last = now;
        state->update(state->ctx, dt);
        state->render(state->ctx);
    }
    state->cleanup(state->ctx);
    SDL_free(state->ctx);
    SDL_free(state);
    game_cleanup(&game);
    
    return 0;
}