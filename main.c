#include <stdio.h>
#include <stdbool.h>
#include <math.h>
// SDL2:
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_mixer.h"

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
        "C-Tale: Meneghetti VS Python",
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

    bool running = SDL_TRUE;
    bool w_pressed = false, s_pressed = false, a_pressed = false, d_pressed = false, spc_pressed = false;
    SDL_Event event;

    // OBJETOS:
    SDL_Texture* meneghetti_texture = criarTextura(renderer, "assets/sprites/characters/sprite-meneghetti-stopped.png");
    SDL_Rect meneghetti_hitbox = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
    Character meneghetti = {meneghetti_texture, meneghetti_hitbox};

    SDL_Texture* scenario_texture = criarTextura(renderer, "assets/sprites/scenario/scenario.png");
    SDL_Rect scenario_hitbox = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
    Character scenario = {scenario_texture, scenario_hitbox};

    int speed = 2;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case(SDL_QUIT):
                    running = false;
                    break;
                case(SDL_KEYDOWN):
                    switch(event.key.keysym.sym) {
                        case(SDLK_w):
                            w_pressed = true;
                            break;
                        case(SDLK_s):
                            s_pressed = true;
                            break;
                        case(SDLK_a):
                            a_pressed = true;
                            break;
                        case(SDLK_d):
                            d_pressed = true;
                            break;
                        case(SDLK_SPACE):
                            spc_pressed = true;
                            break;
                        case(SDLK_ESCAPE):
                            running = false;
                    }
                    break;
                case(SDL_KEYUP):
                    switch(event.key.keysym.sym) {
                        case(SDLK_w):
                            w_pressed = false;
                            break;
                        case(SDLK_s):
                            s_pressed = false;
                            break;
                        case(SDLK_a):
                            a_pressed = false;
                            break;
                        case(SDLK_d):
                            d_pressed = false;
                            break;
                        case(SDLK_SPACE):
                            spc_pressed = false;
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, scenario.texture, NULL, &scenario.colision);
        SDL_RenderCopy(renderer, meneghetti.texture, NULL, &meneghetti.colision);

        if(w_pressed && scenario.colision.y < 0 && meneghetti.colision.y < (SCREEN_HEIGHT / 2) - 16) {
            scenario.colision.y += speed;            
        }
        else if(w_pressed && meneghetti.colision.y > 0) {
            meneghetti.colision.y -= speed;
        }
        if(s_pressed && scenario.colision.y > SCREEN_HEIGHT - scenario.colision.h && meneghetti.colision.y > (SCREEN_HEIGHT / 2) - 16) {
            scenario.colision.y -= speed;
        }
        else if(s_pressed && meneghetti.colision.y < SCREEN_HEIGHT - meneghetti.colision.h) {
            meneghetti.colision.y += speed;
        }
        if(a_pressed && scenario.colision.x < 0 && meneghetti.colision.x < (SCREEN_WIDTH / 2) - 10) {
            scenario.colision.x += speed;
        }
        else if(a_pressed && meneghetti.colision.x > 0) {
            meneghetti.colision.x -= speed;
        }
        if(d_pressed && scenario.colision.x > SCREEN_WIDTH - scenario.colision.w && meneghetti.colision.x > (SCREEN_WIDTH / 2) - 10) {
            scenario.colision.x -= speed;
        }
        else if(d_pressed && meneghetti.colision.x < SCREEN_WIDTH - meneghetti.colision.w) {
            meneghetti.colision.x += speed;
        }

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
