#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_mixer.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define IMAGE_FLAGS IMG_INIT_PNG
#define MIXER_FLAGS (MIX_INIT_MP3 & MIX_INIT_OGG)

#define GAME_TITLE "C-Tale"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Game;
typedef struct {
    SDL_Texture *texture;
    SDL_Rect colision;
} Character;

bool sdl_initialize(Game *game);
// bool load_media(Game *game);
void object_cleanup(Character *obj);
void game_cleanup(Game *game, int exit_status);
SDL_Texture *criarTextura(SDL_Renderer *render, const char *dir);

int main(void) {
    Game game = {
        .renderer = NULL,
        .window = NULL,
    };

    if (sdl_initialize(&game))
        game_cleanup(&game, EXIT_FAILURE);

    SDL_bool running = SDL_TRUE;
    SDL_Event event;

    // OBJETOS:
    Character meneghetti = {
        .texture = criarTextura(game.renderer, "assets/sprites/characters/sprite-meneghetti-stopped.png"),
        .colision = {(SCREEN_WIDTH / 2) - 16, (SCREEN_HEIGHT / 2) - 16, 32, 32},
    };

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                default:
                    break;
                }

            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);

        SDL_RenderClear(game.renderer);

        SDL_RenderCopy(game.renderer, meneghetti.texture, NULL, &meneghetti.colision);

        SDL_RenderPresent(game.renderer);

        SDL_Delay(16);
    }

    object_cleanup(&meneghetti);
    game_cleanup(&game, EXIT_SUCCESS);
    return 0;
}

bool sdl_initialize(Game *game) {
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return true;
    }

    int img_init = IMG_Init(IMAGE_FLAGS);
    if ((img_init & IMAGE_FLAGS) != IMAGE_FLAGS) {
        fprintf(stderr, "Error initializing SDL_image: %s\n", IMG_GetError());
        return true;
    }

    int mix_init = Mix_Init(MIXER_FLAGS);
    if ((mix_init & MIXER_FLAGS) != MIXER_FLAGS) {
        fprintf(stderr, "Error initializing SDL_mixer: %s\n", Mix_GetError());
        return true;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024)) {
        fprintf(stderr, "Error Opening Audio: %s\n", Mix_GetError());
        return true;
    }

    if (TTF_Init()) {
        fprintf(stderr, "Error initializing SDL_ttf: %s\n", TTF_GetError());
        return true;
    }

    game->window = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!game->window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return true;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, 0);
    if (!game->renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return true;
    }

    return false;
}

void game_cleanup(Game *game, int exit_status) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    Mix_CloseAudio();

    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);

    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    exit(exit_status);
}

void object_cleanup(Character *obj) {
    SDL_DestroyTexture(obj->texture);
}

SDL_Texture *criarTextura(SDL_Renderer *render, const char *dir) {
    SDL_Surface *surface = IMG_Load(dir);
    if (!surface) {
        fprintf(stderr, "Error loading image '%s': '%s'\n", dir, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    if (!texture) {
        fprintf(stderr, "Error creating texture: '%s'", SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}