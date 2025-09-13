#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define WINDOW_FLAGS (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define IMAGE_FLAGS (IMG_INIT_PNG)
#define MIXER_FLAGS (MIX_INIT_MP3 | MIX_INIT_OGG)
#define COLLISION_QUANTITY 10
#define DIALOGUE_FONT_SIZE 25
#define MAX_DIALOGUE_CHAR 512
#define MAX_DIALOGUE_STR 20

#define GAME_TITLE "C-Tale: Meneghetti Vs Python"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Game;
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    float sprite_vel; // Alterado para float para moldar a fluidez do movimento com base nos FPS.
    const Uint8 *keystate;
    SDL_Rect interact_collision;
} Character;
typedef struct {
    SDL_Texture **frames; // Array de texturas.
    int count; // Quantidade de frames do array.
} Animation;
typedef struct {
    char *writings[MAX_DIALOGUE_STR];
    int on_frame[MAX_DIALOGUE_STR];
    TTF_Font *text_font;
    SDL_Color text_color;
    SDL_Texture *chars[MAX_DIALOGUE_CHAR];
    int char_count;
    SDL_Rect text_box;
    int cur_str, cur_char;
    double timer;
    bool waiting_for_input;
} Text;

enum direction { UP, DOWN, LEFT, RIGHT, DIR_COUNT };
enum game_states { TITLE_SCREEN, CUTSCENE, OPEN_WORLD, BATTLE_SCREEN };
enum player_states { MOVABLE, DIALOGUE, PAUSE, DEAD };
enum characters { MENEGHETTI, MENEGHETTI_ANGRY, PYTHON};

bool sdl_initialize(Game *game);
// bool load_media(Game *game);
void object_cleanup(SDL_Texture *obj);
void game_cleanup(Game *game, int exit_status);
SDL_Texture *create_texture(SDL_Renderer *render, const char *dir);
SDL_Texture *create_txt(SDL_Renderer *render, char text, TTF_Font *font, SDL_Color color);
void create_dialogue(const Uint8 *keystate, SDL_Renderer *render, Text *text, int *player_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Mix_Chunk **sound);
void reset_dialogue(Text *text);
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

    // PACOTES DE ANIMAÇÃO:
    Animation anim_pack[DIR_COUNT];
    anim_pack[UP].count = 3;
    anim_pack[UP].frames = malloc(sizeof(SDL_Texture*) * anim_pack[UP].count);
    anim_pack[UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back.png");
    anim_pack[UP].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-1.png");
    anim_pack[UP].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-2.png");
    anim_pack[DOWN].count = 3;
    anim_pack[DOWN].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DOWN].count);
    anim_pack[DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front.png");
    anim_pack[DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-1.png");
    anim_pack[DOWN].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-2.png");
    anim_pack[LEFT].count = 2;
    anim_pack[LEFT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[LEFT].count);
    anim_pack[LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left.png");
    anim_pack[LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left-1.png");
    anim_pack[RIGHT].count = 2;
    anim_pack[RIGHT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[RIGHT].count);
    anim_pack[RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right.png");
    anim_pack[RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right-1.png");
    for (int i = 0; i < DIR_COUNT; i++) {
        for (int n = 0; n < anim_pack[i].count; n++) {
            if (!anim_pack[i].frames[n]) {
                fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
                return 1;
            }
        }
    }

    Animation meneghetti_dialogue[2];
    meneghetti_dialogue[0].count = 2;
    meneghetti_dialogue[0].frames = malloc(sizeof(SDL_Texture*) * meneghetti_dialogue[0].count);
    meneghetti_dialogue[0].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-1.png");
    meneghetti_dialogue[0].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-2.png");
    meneghetti_dialogue[1].count = 2;
    meneghetti_dialogue[1].frames = malloc(sizeof(SDL_Texture*) * meneghetti_dialogue[1].count);
    meneghetti_dialogue[1].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-1.png");
    meneghetti_dialogue[1].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-2.png");
    for (int i = 0; i < 2; i++) {
        for (int n = 0; n < meneghetti_dialogue[i].count; n++) {
            if (!meneghetti_dialogue[i].frames[n]) {
                fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
                return 1;
            }
        }
    }

    Animation python_dialogue = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/characters/python-dialogue-1.png"), create_texture(game.renderer, "assets/sprites/characters/python-dialogue-2.png")},
        .count = 2
    };
    if (!python_dialogue.frames[0] || !python_dialogue.frames[1]) {
        fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
        return 1;
    }

    // OBJETOS:
    Character meneghetti = {
        .texture = anim_pack[DOWN].frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32},
        .sprite_vel = 100.0f, // Deve ser par.
        .keystate = SDL_GetKeyboardState(NULL),
        .interact_collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25}
    };
    Character scenario = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/scenario.png"),
        .collision = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };
    if (!scenario.texture) {
        fprintf(stderr, "Error loading scenario: %s\n", SDL_GetError());
        return 1;
    }

    // SONS:
    Mix_Chunk* ambience = Mix_LoadWAV("assets/sounds/sound_effects/in-game/ambient_sound.wav");
    if (!ambience) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(ambience, 50);

    Mix_Chunk* meneghetti_voice = Mix_LoadWAV("assets/sounds/sound_effects/in-game/meneghetti_voice.wav");
    if (!meneghetti_voice) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }

    Mix_Chunk* py_voice = Mix_LoadWAV("assets/sounds/sound_effects/in-game/mr_python_voice.wav");
    if (!py_voice) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }
    Mix_Chunk* dialogue_voices[] = {meneghetti_voice, py_voice};

    // BASES DE TEXTO:
    Text py_dialogue = {
        .writings = {"* Python, hoje sera o dia de sua  ruina.", "* Vem pra cima carecao kkkkkkkk   azidea", "* Cobra miseravel, vai tomar de 7 a 0 comedia."},
        .on_frame = {MENEGHETTI, PYTHON, MENEGHETTI_ANGRY},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_char = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text van_dialogue = {
        .writings = {"* Python safado, essa e a van do mercado livre", "* Isso nao vai ficar assim", "* Preciso do meu cigarro cubano", "* Este e um texto deveras longo para testar a nova funcao de quebra de linha, e os guri. AOOOOOOOOBA, AOOOOOOOOO POTENCIAAAAA AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"},
        .on_frame = {MENEGHETTI_ANGRY, MENEGHETTI_ANGRY, MENEGHETTI, MENEGHETTI},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_char = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!van_dialogue.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Uint32 last_ticks = SDL_GetTicks();
    double anim_timer = 0.0;
    const double anim_interval = 0.12;

    bool interaction_request = false;
    bool ambience_begun = false;
    
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
                    if (player_state == MOVABLE)
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
            if (!ambience_begun) {
                Mix_PlayChannel(2, ambience, -1);
                ambience_begun = true;
            }

            // COLISÕES:
            SDL_Rect boxes[COLLISION_QUANTITY];
            boxes[0] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 739, 981, 221}; // Bloco inferior esquerdo.
            boxes[1] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 384, 607, 153}; // Bloco superior esquerdo (ponte).
            boxes[2] = (SDL_Rect){scenario.collision.x, scenario.collision.y, 253, 384}; // Bloco ao topo esquerdo.
            boxes[3] = (SDL_Rect){scenario.collision.x + 253, scenario.collision.y, 774, 74}; // Bloco ao topo central.
            boxes[4] = (SDL_Rect){scenario.collision.x + 1027, scenario.collision.y, 253, 384}; // Bloco ao topo direito.
            boxes[5] = (SDL_Rect){scenario.collision.x + 673, scenario.collision.y + 384, 607, 158}; // Bloco superior direito (ponte).
            boxes[6] = (SDL_Rect){scenario.collision.x + 1045, scenario.collision.y + 739, 235, 221}; // Bloco inferior esquerdo.
            boxes[7] = (SDL_Rect){scenario.collision.x + 981, scenario.collision.y + 890, 64, 70}; // Bloco do rodapé (lago).
            boxes[8] = (SDL_Rect){scenario.collision.x + 620, scenario.collision.y + 153, 39, 40}; // Bloco do Mr. Python.
            boxes[9] = (SDL_Rect){scenario.collision.x + 758, scenario.collision.y + 592, 64, 4}; // Bloco da Python Van.

            if (player_state == MOVABLE) {
                sprite_update(&scenario, &meneghetti, anim_pack, dt, boxes, COLLISION_QUANTITY, &anim_timer, anim_interval, counters);
            }
            if (interaction_request) {
                if (rects_intersect(&meneghetti.interact_collision, &boxes[8])) {
                    player_state = DIALOGUE;
                }

                if (rects_intersect(&meneghetti.interact_collision, &boxes[9])) {
                    player_state = DIALOGUE;
                }

                interaction_request = false;
            }

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            SDL_RenderCopy(game.renderer, scenario.texture, NULL, &scenario.collision);
            SDL_RenderCopy(game.renderer, meneghetti.texture, NULL, &meneghetti.collision);

            if (player_state == DIALOGUE) {
                if (rects_intersect (&meneghetti.interact_collision, &boxes[8]))
                    create_dialogue(meneghetti.keystate, game.renderer, &py_dialogue, &player_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices);
                if (rects_intersect (&meneghetti.interact_collision, &boxes[9]))
                    create_dialogue(meneghetti.keystate, game.renderer, &van_dialogue, &player_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices);
            }

            SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
            // SDL_RenderDrawRect(game.renderer, &meneghetti.interact_collision);
            // SDL_RenderDrawRect(game.renderer, &meneghetti.collision);
            // SDL_RenderDrawRect(game.renderer, &boxes[8]);
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
    SDL_RenderSetLogicalSize(game->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

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

SDL_Texture *create_texture(SDL_Renderer *render, const char *dir) {
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

SDL_Texture *create_txt(SDL_Renderer *render, char text, TTF_Font *font, SDL_Color color) {
    const char array[2] = {text, '\0'};

    SDL_Surface* surface = TTF_RenderText_Solid(font, array, color);
    if (!surface) {
        fprintf(stderr, "Error loading text surface: %s", TTF_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
    if (!texture) {
        fprintf(stderr, "Error creating texture: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }
    SDL_FreeSurface(surface);
    return texture;
}

void create_dialogue(const Uint8 *keystate, SDL_Renderer *render, Text *text, int *player_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Mix_Chunk **sound) {
    const Uint8 *keys = keystate ? keystate : SDL_GetKeyboardState(NULL);
    
    static double anim_cooldown = 0.2;

    static double e_cooldown = 0.0;
    static bool prev_e_pressed = false;
    e_cooldown += dt;
    bool e_now = keys[SDL_SCANCODE_E];
    
    bool e_pressed = false;
    if (e_now && !prev_e_pressed && e_cooldown >= 0.2) {
        e_pressed = true;
        e_cooldown = 0.0;
    }

    prev_e_pressed = e_now;
    
    int text_amount = 0;
    for (int i = 0; i < MAX_DIALOGUE_STR; i++) {
        if (text->writings[i] == NULL) break;
        text_amount++;
    }
    const double timer_delay = 0.04;
    
    SDL_Rect dialogue_box = {25, SCREEN_HEIGHT - 175, SCREEN_WIDTH - 50, 150};
    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderFillRect(render, &dialogue_box);

    // BORDAS:
    SDL_Rect box_borders[] = {{dialogue_box.x, dialogue_box.y, dialogue_box.w, 5}, {dialogue_box.x, dialogue_box.y, 5, dialogue_box.h}, {dialogue_box.x, dialogue_box.y + 145, dialogue_box.w, 5}, {dialogue_box.x + dialogue_box.w - 5, dialogue_box.y, 5, dialogue_box.h}};
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    SDL_RenderFillRects(render, box_borders, 4);

    SDL_Rect meneghetti_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 72, 96};
    SDL_Rect python_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 96, 96};

    text->text_box.x = dialogue_box.x + 130;
    text->text_box.y = dialogue_box.y + 27;

    static double sfx_timer = 0.0;
    const double sfx_cooldown = 0.03;
    sfx_timer += dt;

    if (!text->waiting_for_input) {
        *anim_timer += dt;
        text->timer += dt;
        if (e_pressed) {
            const char *current = text->writings[text->cur_str];
            while (current[text->cur_char] != '\0' && text->char_count < MAX_DIALOGUE_CHAR) {
                char render_char = current[text->cur_char];
                text->chars[text->char_count] = create_txt(render, render_char, text->text_font, text->text_color);
                if (text->chars[text->char_count]) {
                    text->char_count++;
                }
                text->cur_char++;
            }
            text->waiting_for_input = true;
        }
        else if (text->timer >= timer_delay) {
            text->timer = 0.0;
            const char *current = text->writings[text->cur_str];
            
            if (current[text->cur_char] == '\0') {
                current = text->writings[text->cur_str];
                text->waiting_for_input = true;
            }
            else {
                if (text->char_count < MAX_DIALOGUE_CHAR) {
                    char render_char = current[text->cur_char];
                    text->chars[text->char_count] = create_txt(render, render_char, text->text_font, text->text_color);
                    if (text->chars[text->char_count]) {
                        if (sound && sfx_timer >= sfx_cooldown) {
                            int speaker = text->on_frame[text->cur_str];
                            Mix_Chunk *chunk = NULL;

                            if (speaker == MENEGHETTI || speaker == MENEGHETTI_ANGRY) {
                                chunk = sound[0];
                            }
                            if (speaker == PYTHON) {
                                chunk = sound[1];
                            }

                            if (chunk) {
                                Mix_PlayChannel(1, chunk, 0);
                            }
                            sfx_timer = 0.0;
                        }
                        text->char_count++;
                    }
                }
                text->cur_char++;
            }
        }
    }
    else {
        if (e_pressed) {
            for (int i = 0; i < text->char_count; i++) {
                if (text->chars[i]) {
                    SDL_DestroyTexture(text->chars[i]);
                    text->chars[i] = NULL;
                }
            }
            text->char_count = 0;
            text->cur_char = 0;
            text->cur_str++;

            text->waiting_for_input = false;
            if (text->cur_str >= text_amount) {
                reset_dialogue(text);
                *player_state = MOVABLE;
                return;
            }
        }
    }

    int pos_x = text->text_box.x;
    for (int i = 0; i < text->char_count; i++) {
        SDL_Texture *ct = text->chars[i];
        if (!ct)
            continue;
        
        SDL_Rect dst = {pos_x, text->text_box.y, 13, 25};
        SDL_RenderCopy(render, ct, NULL, &dst);
        
        if (pos_x + dst.w >= dialogue_box.x + dialogue_box.w - 25) {
            pos_x = text->text_box.x;
            text->text_box.y += dst.h;
        }
        else
            pos_x += dst.w;

        if (text->text_box.y >= dialogue_box.y + dialogue_box.h - 27)
            break;
    }

    static int counters[] = {0, 0};
    switch(text->on_frame[text->cur_str]) {
        case MENEGHETTI:
            if (!text->waiting_for_input) {
                while (*anim_timer >= anim_cooldown) {
                    
                    counters[0] = (counters[0] + 1) % meneghetti_face[0].count;
                    *anim_timer = 0.0;
                }
                
                SDL_RenderCopy(render, meneghetti_face[0].frames[counters[0] % meneghetti_face[0].count], NULL, &meneghetti_frame);
            }
            else {
                counters[0] = 0;
                SDL_RenderCopy(render, meneghetti_face[0].frames[0], NULL, &meneghetti_frame);
            }
            break;
        case MENEGHETTI_ANGRY:
            if (!text->waiting_for_input) {
                while (*anim_timer >= anim_cooldown) {
                    
                    counters[0] = (counters[0] + 1) % meneghetti_face[1].count;
                    *anim_timer = 0.0;
                }
                
                SDL_RenderCopy(render, meneghetti_face[1].frames[counters[0] % meneghetti_face[1].count], NULL, &meneghetti_frame);
            }
            else {
                counters[0] = 0;
                SDL_RenderCopy(render, meneghetti_face[1].frames[0], NULL, &meneghetti_frame);
            }
            break;
        case PYTHON:
            if (!text->waiting_for_input) {
                while (*anim_timer >= anim_cooldown) {
                    
                    counters[0] = (counters[0] + 1) % python_face->count;
                    *anim_timer = 0.0;
                }
                
                SDL_RenderCopy(render, python_face->frames[counters[0] % python_face->count], NULL, &python_frame);
            }
            else {
                counters[0] = 0;
                SDL_RenderCopy(render, python_face->frames[0], NULL, &python_frame);
            }
    }
    

}

void reset_dialogue(Text *text) {
    for (int i = 0; i < text->char_count; i++) {
        if (text->chars[i]) {
            SDL_DestroyTexture(text->chars[i]);
            text->chars[i] = NULL;
        }
    }
    text->char_count = 0;
    text->cur_str = 0;
    text->cur_char = 0;
    text->timer = 0.0;
}

void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], int box_count, double *anim_timer, double anim_interval, int counters[]) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    float move_vel = player->sprite_vel * (float)dt;
    int move = (int)roundf(move_vel);

    if (move == 0 && move_vel != 0.0f) move = (move_vel > 0.0f) ? 1 : -1;

    *anim_timer += dt;

    bool moving_up = false, moving_down = false, moving_left = false, moving_right = false;
    
    if (keys[SDL_SCANCODE_W]) {
        if (scenario->collision.y < 0 && player->collision.y < (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = player->collision;
            test.y -= move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->collision.y += move;
                moving_up = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y - 6;
        } else {
            SDL_Rect test = player->collision;
            test.y -= move;
            if (!check_collision(&test, boxes, box_count) && player->collision.y > 0) {
                player->collision.y -= move;
                moving_up = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y - 6;
        }
    }

    if (keys[SDL_SCANCODE_S]) {
        if (scenario->collision.y > -SCREEN_HEIGHT && player->collision.y > (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = player->collision;
            test.y += move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->collision.y -= move;
                moving_down = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y + player->collision.h;
        } else {
            SDL_Rect test = player->collision;
            test.y += move;
            if (!check_collision(&test, boxes, box_count) && player->collision.y < SCREEN_HEIGHT - player->collision.h) {
                player->collision.y += move;
                moving_down = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y + player->collision.h;
        }
    }

    if (keys[SDL_SCANCODE_A]) {
        if (scenario->collision.x < 0 && player->collision.x < (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = player->collision;
            test.x -= move;
            if (!check_collision(&test, boxes, box_count)) {
                scenario->collision.x += move;
                moving_left = true;
            }
            player->interact_collision.x = player->collision.x - player->interact_collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        } else {
            SDL_Rect test = player->collision;
            test.x -= move;
            if (!check_collision(&test, boxes, box_count) && player->collision.x > 0) {
                player->collision.x -= move;
                moving_left = true;
            }
            player->interact_collision.x = player->collision.x - player->interact_collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        }
    }

    if (keys[SDL_SCANCODE_D]) {
        if (scenario->collision.x > -SCREEN_WIDTH && player->collision.x > (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = player->collision;
            test.x += move; // cenário se move pra esquerda -> player aparente se move pra direita
            if (!check_collision(&test, boxes, box_count)) {
                scenario->collision.x -= move;
                moving_right = true;
            }
            player->interact_collision.x = player->collision.x + player->collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        } else {
            SDL_Rect test = player->collision;
            test.x += move;
            if (!check_collision(&test, boxes, box_count) && player->collision.x < SCREEN_WIDTH - player->collision.w) {
                player->collision.x += move;
                moving_right = true;
            }
            player->interact_collision.x = player->collision.x + player->collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
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
