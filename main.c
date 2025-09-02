#include <stdio.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <stdbool.h> 

#define SCREEN_WIDTH 640
#define SCREEN_LENGTH 480

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erro ao inicializar o SDL2: '%s'.", SDL_GetError());
        return 1;
    }
    if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) < 0) {
        printf("Erro ao inicializar o IMG: '%s'.", IMG_GetError());
        return 1;
    }

    SDL_Window* janela = SDL_CreateWindow("Teste: Python é do caralho", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_LENGTH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(janela == NULL) {
        printf("Erro ao criar janela: '%s'.", SDL_GetError());
    }

    SDL_Renderer* renderizador = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
    if(renderizador == NULL) {
        printf("Erro ao carregar renderizador: '%s'.", SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    SDL_RenderSetLogicalSize(renderizador, SCREEN_WIDTH, SCREEN_LENGTH); // Isso é para a gente trabalhar em um canva 640x480, devido aos sprites em baixa resolução.

    bool rodando = true;
    SDL_Event evento;

    SDL_Rect quadrado = {};

    while(rodando) {
        while(SDL_PollEvent(&evento)) {
            if(evento.type == SDL_QUIT) {
                rodando = false;
            }
        }

        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
        SDL_RenderClear(renderizador);

        SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
        SDL_RenderFillRect(renderizador, &quadrado);

        SDL_RenderPresent(renderizador);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(janela);
    SDL_Quit();
    IMG_Quit();

    return 0;
}
