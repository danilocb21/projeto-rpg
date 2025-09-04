#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

SDL_Texture* criarTextura(SDL_Renderer* render, const char* dir)
{
    SDL_Surface* surface = IMG_Load(dir);
    if(!surface) {
        SDL_Log("Erro ao carregar imagem '%s': '%s'", dir, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
    if(!texture) {
        SDL_Log("Erro ao criar textura: '%s'", SDL_GetError());
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}

typedef struct {
    SDL_Texture* texture;
    SDL_Rect colision;
} Character;

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Tela Preta",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_bool running = SDL_TRUE;
    SDL_Event event;

    // OBJETOS:
    SDL_Texture* meneghetti_texture = criarTextura(renderer, "assets/sprites/characters/sprite-meneghetti-stopped.png");
    SDL_Rect meneghetti_hitbox = {(SCREEN_WIDTH / 2) - 16, (SCREEN_HEIGHT / 2) - 16, 32, 32};
    Character meneghetti = {meneghetti_texture, meneghetti_hitbox};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = SDL_FALSE;
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = SDL_FALSE;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, meneghetti.texture, NULL, &meneghetti.colision);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
