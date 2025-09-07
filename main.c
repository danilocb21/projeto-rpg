#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define WINDOW_FLAGS SDL_WINDOW_SHOWN
#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define IMAGE_FLAGS (IMG_INIT_PNG)
#define MIXER_FLAGS (MIX_INIT_MP3 | MIX_INIT_OGG)

#define GAME_TITLE "C-Tale: Meneghetti Vs Python"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Game;
typedef struct {
    SDL_Texture *texture;
    SDL_Rect colision;
    int sprite_vel;
    const Uint8 *keystate;
} Character;

bool sdl_initialize(Game *game);
// bool load_media(Game *game);
void object_cleanup(Character *obj);
void game_cleanup(Game *game, int exit_status);
SDL_Texture *criarTextura(SDL_Renderer *render, const char *dir);
void sprite_update(Character *scenario, Character *player, SDL_Rect *box);

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

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
        .colision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32},
        .sprite_vel = 4, // Deve ser par.
        .keystate = SDL_GetKeyboardState(NULL),
    };
    Character scenario = {
        .texture = criarTextura(game.renderer, "assets/sprites/scenario/scenario.png"),
        .colision = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2},
    };

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = SDL_FALSE;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    running = SDL_FALSE;
                    break;
                default:
                    break;
                }

            default:
                break;
            }
        }

        // COLISÕES:
        SDL_Rect lower_left_col = {scenario.colision.x, (scenario.colision.y + scenario.colision.h) - 22, 982, 220};

        sprite_update(&scenario, &meneghetti, &lower_left_col);

        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);

        SDL_RenderClear(game.renderer);

        SDL_RenderCopy(game.renderer, scenario.texture, NULL, &scenario.colision);
        SDL_RenderCopy(game.renderer, meneghetti.texture, NULL, &meneghetti.colision);

        SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 0);
        SDL_RenderDrawRect(game.renderer, &lower_left_col);

        SDL_RenderPresent(game.renderer);

        SDL_Delay(16);
    }

    object_cleanup(&meneghetti);
    object_cleanup(&scenario);
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
                                    SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_FLAGS);
    if (!game->window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return true;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, RENDERER_FLAGS);
    if (!game->renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return true;
    }

    SDL_Surface* icon = SDL_LoadBMP("assets/sprites/misc/icon.bmp");
    if(!icon) {
        fprintf(stderr, "Error loading icon: %s\n", SDL_GetError());
        return true;
    }
    SDL_SetWindowIcon(game->window, icon);
    SDL_FreeSurface(icon);

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

void sprite_update(Character *scenario, Character *player, SDL_Rect *box) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    int speed = player->sprite_vel;

    if (keys[SDL_SCANCODE_W] && scenario->colision.y < 0 && player->colision.y < (SCREEN_HEIGHT / 2) - 16) {
            scenario->colision.y += speed;
    } else if (keys[SDL_SCANCODE_W] && player->colision.y > 0) {
        player->colision.y -= speed;
    }

    if (keys[SDL_SCANCODE_S] && scenario->colision.y > -SCREEN_HEIGHT && player->colision.y > (SCREEN_HEIGHT / 2) - 16 && player->colision.y + player->colision.h < box->y) {
        scenario->colision.y -= speed;
    } else if (keys[SDL_SCANCODE_S] && player->colision.y < SCREEN_HEIGHT - player->colision.h && player->colision.y + player->colision.h < box->y) {
        player->colision.y += speed;
    }

    if (keys[SDL_SCANCODE_A] && scenario->colision.x < 0 && player->colision.x < (SCREEN_WIDTH / 2) - 10) {
        scenario->colision.x += speed;
    } else if (keys[SDL_SCANCODE_A] && player->colision.x > 0) {
        player->colision.x -= speed;
    }

    if (keys[SDL_SCANCODE_D] && scenario->colision.x > -SCREEN_WIDTH && player->colision.x > (SCREEN_WIDTH / 2) - 10) {
        scenario->colision.x -= speed;
    } else if (keys[SDL_SCANCODE_D] && player->colision.x < SCREEN_WIDTH - player->colision.w) {
        player->colision.x += speed;
    }
}

bool colision_check(Character *player, SDL_Rect *box) {
    int top_left_player, top_left_box;
    int top_left_player_y, top_left_box_y;
    int top_right_player, top_right_box;
    int bottom_left_player, bottom_left_box;

    top_left_player = player->colision.x;
    top_left_player_y = player->colision.y;
    top_right_player = player->colision.x + player->colision.w;
    bottom_left_player = player->colision.y + player->colision.h;

    top_left_box = box->x;
    top_left_box_y = box->y;
    top_right_box = box->x + box->w;
    bottom_left_box = box->y + box->h;

    
}