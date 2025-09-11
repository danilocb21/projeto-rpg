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
#define COLLISION_QUANTITY 10

#define GAME_TITLE "C-Tale: Meneghetti Vs Python"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Game;
typedef struct {
    SDL_Texture *texture;
    SDL_Rect colision;
    float sprite_vel; // Alterado para float para moldar a fluidez do movimento com base nos FPS.
    const Uint8 *keystate;
    SDL_Rect interact_colision;
} Character;
typedef struct {
    SDL_Texture **frames; // Array de texturas.
    int count; // Quantidade de frames do array.
} Animation;

enum direction { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, DIR_COUNT = 4 };
enum game_states { TITLE_SCREEN, CUTSCENE, OPEN_WORLD, BATTLE_SCREEN };
enum player_states { MOVABLE, DIALOGUE, PAUSE, DEAD };

bool sdl_initialize(Game *game);
// bool load_media(Game *game);
void object_cleanup(SDL_Texture *obj);
void game_cleanup(Game *game, int exit_status);
SDL_Texture *criarTextura(SDL_Renderer *render, const char *dir);
void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], int box_count, double *anim_timer, double anim_interval, int counters[]);
bool rects_intersect(SDL_Rect *a, SDL_Rect *b);
bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count);

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    int counters[] = {0, 0, 0, 0};
    int game_state = OPEN_WORLD;
    int player_state = MOVABLE;

    Game game = {
        .renderer = NULL,
        .window = NULL,
    };

    if (sdl_initialize(&game))
        game_cleanup(&game, EXIT_FAILURE);

    SDL_bool running = SDL_TRUE;
    SDL_Event event;

    // PACOTE DE ANIMAÇÃO:
    Animation anim_pack[DIR_COUNT];
    anim_pack[UP].count = 3;
    anim_pack[UP].frames = malloc(sizeof(SDL_Texture*) * anim_pack[UP].count);
    anim_pack[UP].frames[0] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-back.png");
    anim_pack[UP].frames[1] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-back-1.png");
    anim_pack[UP].frames[2] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-back-2.png");
    anim_pack[DOWN].count = 3;
    anim_pack[DOWN].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DOWN].count);
    anim_pack[DOWN].frames[0] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-front.png");
    anim_pack[DOWN].frames[1] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-front-1.png");
    anim_pack[DOWN].frames[2] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-front-2.png");
    anim_pack[LEFT].count = 2;
    anim_pack[LEFT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[LEFT].count);
    anim_pack[LEFT].frames[0] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-left.png");
    anim_pack[LEFT].frames[1] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-left-1.png");
    anim_pack[RIGHT].count = 2;
    anim_pack[RIGHT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[RIGHT].count);
    anim_pack[RIGHT].frames[0] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-right.png");
    anim_pack[RIGHT].frames[1] = criarTextura(game.renderer, "assets/sprites/characters/meneghetti-right-1.png");
    for (int i = 0; i < DIR_COUNT; i++) {
        for (int n = 0; n < anim_pack[i].count; n++) {
            if (!anim_pack[i].frames[n]) {
                fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
                return 1;
            }
        }
    }

    // OBJETOS:
    Character meneghetti = {
        .texture = anim_pack[DOWN].frames[0],
        .colision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32},
        .sprite_vel = 120.0f, // Deve ser par.
        .keystate = SDL_GetKeyboardState(NULL),
        .interact_colision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 19},
    };
    Character scenario = {
        .texture = criarTextura(game.renderer, "assets/sprites/scenario/scenario.png"),
        .colision = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2},
    };
    if (!scenario.texture) {
        fprintf(stderr, "Error loading scenario: %s\n", SDL_GetError());
        return 1;
    }

    // SONS:
    Mix_Chunk* lapada = Mix_LoadWAV("assets/sounds/sound_effects/in-game/car_door.wav");
    if (!lapada) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(lapada, MIX_MAX_VOLUME);

    Uint32 last_ticks = SDL_GetTicks();
    double anim_timer = 0.0;
    const double anim_interval = 0.12;

    bool interaction_request = false;

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
                case SDL_SCANCODE_E:
                    interaction_request = true;
                    break;
                default:
                    break;
                }

            default:
                break;
            }
        }

        Uint32 now = SDL_GetTicks();
        double dt = (now - last_ticks) / 1000.0;
        if (dt > 0.25) dt = 0.25;
        last_ticks = now;

        if (game_state == OPEN_WORLD) {
            // COLISÕES:
            SDL_Rect boxes[COLLISION_QUANTITY];
            boxes[0] = (SDL_Rect){scenario.colision.x, scenario.colision.y + 739, 981, 221}; // Bloco inferior esquerdo.
            boxes[1] = (SDL_Rect){scenario.colision.x, scenario.colision.y + 384, 607, 153}; // Bloco superior esquerdo (ponte).
            boxes[2] = (SDL_Rect){scenario.colision.x, scenario.colision.y, 253, 384}; // Bloco ao topo esquerdo.
            boxes[3] = (SDL_Rect){scenario.colision.x + 253, scenario.colision.y, 774, 74}; // Bloco ao topo central.
            boxes[4] = (SDL_Rect){scenario.colision.x + 1027, scenario.colision.y, 253, 384}; // Bloco ao topo direito.
            boxes[5] = (SDL_Rect){scenario.colision.x + 673, scenario.colision.y + 384, 607, 158}; // Bloco superior direito (ponte).
            boxes[6] = (SDL_Rect){scenario.colision.x + 1045, scenario.colision.y + 739, 235, 221}; // Bloco inferior esquerdo.
            boxes[7] = (SDL_Rect){scenario.colision.x + 981, scenario.colision.y + 890, 64, 70}; // Bloco do rodapé (lago).
            boxes[8] = (SDL_Rect){scenario.colision.x + 620, scenario.colision.y + 153, 39, 40}; // Bloco do Mr. Python.
            boxes[9] = (SDL_Rect){scenario.colision.x + 758, scenario.colision.y + 592, 64, 4}; // Bloco da Python Van.

            if (player_state == MOVABLE) {
                sprite_update(&scenario, &meneghetti, anim_pack, dt, boxes, COLLISION_QUANTITY, &anim_timer, anim_interval, counters);
            }
            if (interaction_request) {
                if (rects_intersect(&meneghetti.interact_colision, &boxes[9])) {
                    printf("Tomi!!");
                    Mix_PlayChannel(1, lapada, 1);
                }

                interaction_request = false;
            }

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            SDL_RenderCopy(game.renderer, scenario.texture, NULL, &scenario.colision);
            SDL_RenderCopy(game.renderer, meneghetti.texture, NULL, &meneghetti.colision);

            SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(game.renderer, &meneghetti.interact_colision);
        }

        SDL_RenderPresent(game.renderer);

        SDL_Delay(1);
    }

    for (int i = 0; i < DIR_COUNT; i++) {
        for (int n = 0; n < anim_pack[i].count; n++) {
            object_cleanup(anim_pack[i].frames[n]);
        }
        free(anim_pack[i].frames);
    }
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
    Mix_AllocateChannels(16);

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

    SDL_Surface* icon = SDL_LoadBMP("assets/sprites/hud/icon.bmp");
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

void object_cleanup(SDL_Texture *obj) {
    SDL_DestroyTexture(obj);
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
        SDL_FreeSurface(surface);
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}

void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], int box_count, double *anim_timer, double anim_interval, int counters[]) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    float move_vel = player->sprite_vel * (float)dt;
    int move = (int)roundf(move_vel);

    if (move == 0 && move_vel != 0.0f) move = (move_vel > 0.0f) ? 1 : -1;

    *anim_timer += dt;

    bool moving_up = false, moving_down = false, moving_left = false, moving_right = false;
    
    if (keys[SDL_SCANCODE_W]) {
        if (scenario->colision.y < 0 && player->colision.y < (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = player->colision;
            test.y -= move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->colision.y += move;
                moving_up = true;
            }
            player->interact_colision.x = player->colision.x;
            player->interact_colision.y = player->colision.y - 2;
        } else {
            SDL_Rect test = player->colision;
            test.y -= move;
            if (!check_collision(&test, boxes, box_count) && player->colision.y > 0) {
                player->colision.y -= move;
                moving_up = true;
            }
            player->interact_colision.x = player->colision.x;
            player->interact_colision.y = player->colision.y - 2;
        }
    }

    if (keys[SDL_SCANCODE_S]) {
        if (scenario->colision.y > -SCREEN_HEIGHT && player->colision.y > (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = player->colision;
            test.y += move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->colision.y -= move;
                moving_down = true;
            }
            player->interact_colision.x = player->colision.x;
            player->interact_colision.y = player->colision.y + player->colision.h;
        } else {
            SDL_Rect test = player->colision;
            test.y += move;
            if (!check_collision(&test, boxes, box_count) && player->colision.y < SCREEN_HEIGHT - player->colision.h) {
                player->colision.y += move;
                moving_down = true;
            }
            player->interact_colision.x = player->colision.x;
            player->interact_colision.y = player->colision.y + player->colision.h;
        }
    }

    if (keys[SDL_SCANCODE_A]) {
        if (scenario->colision.x < 0 && player->colision.x < (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = player->colision;
            test.x -= move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->colision.x += move;
                moving_left = true;
            }
            player->interact_colision.x = player->colision.x - player->interact_colision.w;
            player->interact_colision.y = (player->colision.y + player->colision.h) - player->interact_colision.h;
        } else {
            SDL_Rect test = player->colision;
            test.x -= move;
            if (!check_collision(&test, boxes, box_count) && player->colision.x > 0) {
                player->colision.x -= move;
                moving_left = true;
            }
            player->interact_colision.x = player->colision.x - player->interact_colision.w;
            player->interact_colision.y = (player->colision.y + player->colision.h) - player->interact_colision.h;
        }
    }

    if (keys[SDL_SCANCODE_D]) {
        if (scenario->colision.x > -SCREEN_WIDTH && player->colision.x > (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = player->colision;
            test.x += move; // cenário se move pra esquerda -> player aparente se move pra direita
            if (!check_collision(&test, boxes, box_count)) {
                scenario->colision.x -= move;
                moving_right = true;
            }
            player->interact_colision.x = player->colision.x + player->colision.w;
            player->interact_colision.y = (player->colision.y + player->colision.h) - player->interact_colision.h;
        } else {
            SDL_Rect test = player->colision;
            test.x += move;
            if (!check_collision(&test, boxes, box_count) && player->colision.x < SCREEN_WIDTH - player->colision.w) {
                player->colision.x += move;
                moving_right = true;
            }
            player->interact_colision.x = player->colision.x + player->colision.w;
            player->interact_colision.y = (player->colision.y + player->colision.h) - player->interact_colision.h;
        }
    }

    if (moving_up) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[UP].frames[ counters[UP] % animation[UP].count ];
            counters[UP] = (counters[UP] + 1) % animation[UP].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_down) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[DOWN].frames[ counters[DOWN] % animation[DOWN].count ];
            counters[DOWN] = (counters[DOWN] + 1) % animation[DOWN].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_left) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[LEFT].frames[ counters[LEFT] % animation[LEFT].count ];
            counters[LEFT] = (counters[LEFT] + 1) % animation[LEFT].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_right) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[RIGHT].frames[ counters[RIGHT] % animation[RIGHT].count ];
            counters[RIGHT] = (counters[RIGHT] + 1) % animation[RIGHT].count;
            *anim_timer = 0.0;
        }
    }
}

bool rects_intersect(SDL_Rect *a, SDL_Rect *b) {
    int leftX_A, leftX_B;
    int topY_A, topY_B;
    int rightX_A, rightX_B;
    int bottomY_A, bottomY_B;

    leftX_A = a->x;
    topY_A = a->y;
    rightX_A = a->x + a->w;
    bottomY_A = a->y + a->h;

    leftX_B = b->x;
    topY_B = b->y;
    rightX_B = b->x + b->w;
    bottomY_B = b->y + b->h;

    if (leftX_A >= rightX_B)
        return false;

    if (topY_A >= bottomY_B)
        return false;

    if (rightX_A <= leftX_B)
        return false;

    if (bottomY_A <= topY_B)
        return false;

    return true;
}

bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count) {
    for (int i = 0; i < box_count; i++) {
        if (rects_intersect(player, &boxes[i])) return true;
    }

    return false;
}
