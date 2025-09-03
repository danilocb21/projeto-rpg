#include <stdio.h>
#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define MAX_CLIQUES 100

typedef struct {
    SDL_Rect transform;
    float velocity_x;
    float velocity_y;
    double angle;
} Personagem;

SDL_Texture* carregarTextura(SDL_Renderer* rend, char* dir) {
    
    SDL_Surface* surface = IMG_Load(dir);
    if(!surface) {
        return NULL;
    }
    else {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(rend, surface);
        return texture;
    }

}

int main(int argc, char* argv[]) {
    (void) argc; (void) argv;

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    int imgFlags = IMG_INIT_PNG;
    if((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        printf("IMG_Init error: %s\n", IMG_GetError());
    }

    SDL_Window* janela = SDL_CreateWindow(
        "Teste: cliques + icone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if(!janela) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface* icon_surf = IMG_Load("assets/sprites/misc/icon.png");
    if(!icon_surf) {
        icon_surf = SDL_LoadBMP("assets/sprites/misc/icon.bmp");
    }
    if(icon_surf) {
        SDL_SetWindowIcon(janela, icon_surf);
        SDL_FreeSurface(icon_surf);
    } else {
        printf("Falha ao carregar ícone: IMG_Error='%s' SDL_Error='%s'\n", IMG_GetError(), SDL_GetError());
    }

    SDL_Renderer* renderizador = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
    if(!renderizador) {
        printf("Erro ao criar renderizador: %s\n", SDL_GetError());
        SDL_DestroyWindow(janela);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Surface* s0 = IMG_Load("assets/sprites/misc/soul.png");
    SDL_Surface* s1 = IMG_Load("assets/sprites/hud/button-fight.png");
    SDL_Surface* s2 = IMG_Load("assets/sprites/characters/sprite-meneghetti-stopped.png");
    if(!s0 || !s1 || !s2) {
        printf("Erro ao carregar imagens: IMG_Error='%s'\n", IMG_GetError());
        if (s0) SDL_FreeSurface(s0);
        if (s1) SDL_FreeSurface(s1);
        if (s2) SDL_FreeSurface(s2);
        SDL_DestroyRenderer(renderizador);
        SDL_DestroyWindow(janela);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* heart = carregarTextura(renderizador, "assets/sprites/misc/soul.png");
    SDL_Texture* clique = SDL_CreateTextureFromSurface(renderizador, s1);
    SDL_Texture* menegas = SDL_CreateTextureFromSurface(renderizador, s2);
    SDL_FreeSurface(s0);
    SDL_FreeSurface(s1);
    SDL_FreeSurface(s2);
    if(!heart || !clique || !menegas) {
        printf("Erro ao criar textures: %s\n", SDL_GetError());
        if (heart) SDL_DestroyTexture(heart);
        if (clique) SDL_DestroyTexture(clique);
        if (menegas) SDL_DestroyTexture(menegas);
        SDL_DestroyRenderer(renderizador);
        SDL_DestroyWindow(janela);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize(renderizador, SCREEN_WIDTH, SCREEN_HEIGHT);

    bool rodando = true;
    SDL_Event evento;

    SDL_Rect bordas[] = { {0, 0, 10, SCREEN_HEIGHT}, {0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10}, {SCREEN_WIDTH - 10, 0, 10, SCREEN_HEIGHT}, {0, 0, SCREEN_WIDTH, 10} };
    SDL_Rect player = { (SCREEN_WIDTH / 2) - 16, (SCREEN_HEIGHT / 2) - 16, 32, 32};
    SDL_Rect quadrado = { (SCREEN_WIDTH / 2) - 30, (SCREEN_HEIGHT / 2) - 30, 60, 60 };
    Personagem personagem = { quadrado, 2, 2, 0 };

    SDL_Rect cliques[MAX_CLIQUES];
    int total_cliques = 0;

    bool up = false, down = false, right = false, left = false;

    while(rodando) {
        while(SDL_PollEvent(&evento)) {
            switch (evento.type) {
                case SDL_QUIT:
                    rodando = false;
                    break;

                case SDL_MOUSEBUTTONDOWN: {
                    int win_w = 0, win_h = 0;
                    SDL_GetWindowSize(janela, &win_w, &win_h);
                    int mx = evento.button.x;
                    int my = evento.button.y;
                    if(win_w > 0 && win_h > 0) {
                        float sx = (float)SCREEN_WIDTH / (float)win_w;
                        float sy = (float)SCREEN_HEIGHT / (float)win_h;
                        mx = (int)(mx * sx + 0.5f);
                        my = (int)(my * sy + 0.5f);
                    }
                    if(total_cliques < MAX_CLIQUES) {
                        cliques[total_cliques++] = (SDL_Rect){ mx - 8, my - 8, 64, 64 };
                    }
                    break;
                }

                case SDL_KEYDOWN:
                    switch(evento.key.keysym.sym) {
                        case(SDLK_w):
                            up = true;
                            break;
                        case(SDLK_s):
                            down = true;
                            break;
                        case(SDLK_a):
                            left = true;
                            break;
                        case(SDLK_d):
                            right = true;
                            break;
                    }
                    break;

                case SDL_KEYUP:
                    switch(evento.key.keysym.sym) {
                        case(SDLK_w):
                            up = false;
                            break;
                        case(SDLK_s):
                            down = false;
                            break;
                        case(SDLK_a):
                            left = false;
                            break;
                        case(SDLK_d):
                            right = false;
                            break;
                    }
                    break;

                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
        SDL_RenderClear(renderizador);

        for(int i = 0; i < 4; i++) {
            SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
            SDL_RenderFillRect(renderizador, &bordas[i]);
        }

        SDL_Point centro = { quadrado.w / 2, quadrado.h / 2 };
        SDL_RenderCopyEx(renderizador, heart, NULL, &personagem.transform, personagem.angle, &centro, 0);

        SDL_RenderCopy(renderizador, menegas, NULL, &player);
        if(up && player.y > 10) player.y -= 2;
        if(down && player.y < SCREEN_HEIGHT - 42) player.y += 2;
        if(left && player.x > 10) player.x -= 2;
        if(right && player.x < SCREEN_WIDTH - 42) player.x += 2;

        for (int i = 0; i < total_cliques; ++i) {
            SDL_RenderCopy(renderizador, clique, NULL, &cliques[i]);
        }

        SDL_RenderPresent(renderizador);

        personagem.transform.x += personagem.velocity_x;
        personagem.transform.y += personagem.velocity_y;
        personagem.angle += 2;

        if (personagem.transform.x > SCREEN_WIDTH - personagem.transform.w - 10 || personagem.transform.x < 10) personagem.velocity_x *= -1;
        if (personagem.transform.y > SCREEN_HEIGHT - personagem.transform.h - 10 || personagem.transform.y < 10) personagem.velocity_y *= -1;

        SDL_Delay(16);
    }

    SDL_DestroyTexture(heart);
    SDL_DestroyTexture(clique);
    SDL_DestroyRenderer(renderizador);

    IMG_Quit();
    SDL_DestroyWindow(janela);
    SDL_Quit();
    return 0;
}
