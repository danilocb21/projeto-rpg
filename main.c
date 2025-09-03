#include <stdio.h>
#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define MAX_CLIQUES 100

typedef struct {
    SDL_Rect transform;
    SDL_Texture* texture;
    float velocity_x;
    float velocity_y;
    double angle;
} Personagem;

SDL_Texture* carregarTextura(SDL_Renderer* rend, const char* dir) {
    SDL_Surface* surface = IMG_Load(dir);
    if(!surface) {
        SDL_Log("IMG_Load failed for %s: %s", dir, IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_FreeSurface(surface);
    if(!texture) SDL_Log("CreateTextureFromSurface failed: %s", SDL_GetError());
    return texture;
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
        SDL_Quit();
        return 1;
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

    // caminhos dos sprites
    const char* front_paths[] = {
        "assets/sprites/characters/sprite-meneghetti-stopped.png",
        "assets/sprites/characters/sprite-meneghetti-walking-1.png",
        "assets/sprites/characters/sprite-meneghetti-walking-2.png"
    };
    const int front_count = sizeof(front_paths)/sizeof(front_paths[0]);

    const char* back_paths[] = {
        "assets/sprites/characters/sprite-meneghetti-back-stopped.png",
        "assets/sprites/characters/sprite-meneghetti-back-walking-1.png",
        "assets/sprites/characters/sprite-meneghetti-back-walking-2.png"
    };
    const int back_count = sizeof(back_paths)/sizeof(back_paths[0]);

    const char* left_paths[] = {
        "assets/sprites/characters/sprite-meneghetti-left-side-stopped.png",
        "assets/sprites/characters/sprite-meneghetti-left-side-walking-1.png"
    };
    const int left_count = sizeof(left_paths)/sizeof(left_paths[0]);

    const char* right_paths[] = {
        "assets/sprites/characters/sprite-meneghetti-right-side-stopped.png",
        "assets/sprites/characters/sprite-meneghetti-right-side-walking-1.png"
    };
    const int right_count = sizeof(right_paths)/sizeof(right_paths[0]);

    // carregar texturas uma vez
    SDL_Texture* front_tex[3] = {0};
    SDL_Texture* back_tex[3]  = {0};
    SDL_Texture* left_tex[2]  = {0};
    SDL_Texture* right_tex[2] = {0};

    for(int i=0;i<front_count;i++) front_tex[i] = carregarTextura(renderizador, front_paths[i]);
    for(int i=0;i<back_count;i++)  back_tex[i]  = carregarTextura(renderizador, back_paths[i]);
    for(int i=0;i<left_count;i++)  left_tex[i]  = carregarTextura(renderizador, left_paths[i]);
    for(int i=0;i<right_count;i++) right_tex[i] = carregarTextura(renderizador, right_paths[i]);

    SDL_Texture* heart = carregarTextura(renderizador, "assets/sprites/misc/soul.png");
    SDL_Texture* clique = carregarTextura(renderizador, "assets/sprites/hud/button-fight.png");
    if(!heart || !clique || !front_tex[0]) {
        printf("Erro ao criar textures (verifique caminhos). IMG_Error='%s' SDL_Error='%s'\n", IMG_GetError(), SDL_GetError());
        // liberar o que foi criado e sair
        if(heart) SDL_DestroyTexture(heart);
        if(clique) SDL_DestroyTexture(clique);
        for(int i=0;i<front_count;i++) if(front_tex[i]) SDL_DestroyTexture(front_tex[i]);
        for(int i=0;i<back_count;i++)  if(back_tex[i])  SDL_DestroyTexture(back_tex[i]);
        for(int i=0;i<left_count;i++)  if(left_tex[i])  SDL_DestroyTexture(left_tex[i]);
        for(int i=0;i<right_count;i++) if(right_tex[i]) SDL_DestroyTexture(right_tex[i]);
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
    Personagem personagem = { quadrado, heart, 2, 2, 0 };

    // personagem animado (meneghetti) usa player como fonte de posição
    Personagem meneghetti = { player, front_tex[0], 2, 2, 0 };

    SDL_Rect cliques[MAX_CLIQUES];
    int total_cliques = 0;

    bool up = false, down = false, right = false, left = false;

    enum { DIR_FRONT=0, DIR_BACK=1, DIR_LEFT=2, DIR_RIGHT=3 } dir = DIR_FRONT;
    int frame_index = 0;
    Uint32 last_anim_time = SDL_GetTicks();
    const Uint32 ANIM_DELAY = 150; // ms entre frames

    SDL_Point centro = { quadrado.w / 2, quadrado.h / 2 };

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
                        case(SDLK_w): up = true; break;
                        case(SDLK_s): down = true; break;
                        case(SDLK_a): left = true; break;
                        case(SDLK_d): right = true; break;
                    }
                    break;
                case SDL_KEYUP:
                    switch(evento.key.keysym.sym) {
                        case(SDLK_w): up = false; break;
                        case(SDLK_s): down = false; break;
                        case(SDLK_a): left = false; break;
                        case(SDLK_d): right = false; break;
                    }
                    break;
                default:
                    break;
            }
        }

        // MOVIMENTO: atualiza player (fonte única)
        bool moving = false;
        if(up && player.y > 10)   { player.y -= 2; dir = DIR_BACK; moving = true; }
        if(down && player.y < SCREEN_HEIGHT - player.h - 10) { player.y += 2; dir = DIR_FRONT; moving = true; }
        if(left && player.x > 10) { player.x -= 2; dir = DIR_LEFT; moving = true; }
        if(right && player.x < SCREEN_WIDTH - player.w - 10) { player.x += 2; dir = DIR_RIGHT; moving = true; }

        // animação por tempo
        Uint32 now = SDL_GetTicks();
        if(moving) {
            if(now - last_anim_time >= ANIM_DELAY) {
                last_anim_time = now;
                // incrementa frame conforme direção
                if(dir == DIR_FRONT) frame_index = (frame_index + 1) % front_count;
                else if(dir == DIR_BACK) frame_index = (frame_index + 1) % back_count;
                else if(dir == DIR_LEFT) frame_index = (frame_index + 1) % left_count;
                else if(dir == DIR_RIGHT) frame_index = (frame_index + 1) % right_count;
            }
        } else {
            // parado -> frame 0
            frame_index = 0;
        }

        // escolhe textura atual conforme direção + frame_index
        SDL_Texture* current = NULL;
        if(dir == DIR_FRONT) current = front_tex[frame_index % front_count];
        else if(dir == DIR_BACK) current = back_tex[frame_index % back_count];
        else if(dir == DIR_LEFT) current = left_tex[frame_index % left_count];
        else if(dir == DIR_RIGHT) current = right_tex[frame_index % right_count];

        // atualiza transform usado para render
        meneghetti.transform = player;
        meneghetti.texture = current;

        // render
        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
        SDL_RenderClear(renderizador);

        for(int i = 0; i < 4; i++) {
            SDL_SetRenderDrawColor(renderizador, 255, 255, 255, 255);
            SDL_RenderFillRect(renderizador, &bordas[i]);
        }

        SDL_RenderCopyEx(renderizador, personagem.texture, NULL, &personagem.transform, personagem.angle, &centro, 0);

        if(meneghetti.texture) SDL_RenderCopy(renderizador, meneghetti.texture, NULL, &meneghetti.transform);

        for (int i = 0; i < total_cliques; ++i) {
            SDL_RenderCopy(renderizador, clique, NULL, &cliques[i]);
        }

        SDL_RenderPresent(renderizador);

        // move personagem "quadrado" automaticamente (só se quiser)
        personagem.transform.x += personagem.velocity_x;
        personagem.transform.y += personagem.velocity_y;
        personagem.angle += 2;

        if (personagem.transform.x > SCREEN_WIDTH - personagem.transform.w - 10 || personagem.transform.x < 10) personagem.velocity_x *= -1;
        if (personagem.transform.y > SCREEN_HEIGHT - personagem.transform.h - 10 || personagem.transform.y < 10) personagem.velocity_y *= -1;

        SDL_Delay(16);
    }

    // liberar texturas
    SDL_DestroyTexture(heart);
    SDL_DestroyTexture(clique);
    for(int i=0;i<front_count;i++) if(front_tex[i]) SDL_DestroyTexture(front_tex[i]);
    for(int i=0;i<back_count;i++)  if(back_tex[i])  SDL_DestroyTexture(back_tex[i]);
    for(int i=0;i<left_count;i++)  if(left_tex[i])  SDL_DestroyTexture(left_tex[i]);
    for(int i=0;i<right_count;i++) if(right_tex[i]) SDL_DestroyTexture(right_tex[i]);

    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(janela);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
