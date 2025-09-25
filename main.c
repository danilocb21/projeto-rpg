#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
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
#define COLLISION_QUANTITY 12
#define SURFACE_QUANTITY 13
#define DIALOGUE_FONT_SIZE 25
#define MAX_DIALOGUE_CHAR 512
#define MAX_DIALOGUE_STR 20
#define DIR_COUNT 4

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
    int health;
    int strength;
    int facing;
    int counters[DIR_COUNT];
} Character;
typedef struct {
    SDL_Texture **frames; // Array de texturas.
    int count; // Quantidade de frames do array.
} Animation;
typedef struct {
    int counter;
    double timer;
} AnimState;
typedef struct {
    char *writings[MAX_DIALOGUE_STR];
    int on_frame[MAX_DIALOGUE_STR];
    TTF_Font *text_font;
    SDL_Color text_color;
    SDL_Texture *chars[MAX_DIALOGUE_CHAR];
    char chars_string[MAX_DIALOGUE_CHAR][5];
    int char_count;
    SDL_Rect text_box;
    int cur_str, cur_byte;
    double timer;
    bool waiting_for_input;
} Text;
typedef struct {
    AnimState anim_state;
    SDL_Rect collision;
    SDL_Texture* current;
} Prop;
typedef struct {
    SDL_Texture* texture;
    SDL_Rect *collisions;
} RenderItem;
typedef struct {
    SDL_Texture* image;
    Text* text;
    double duration;
} CutsceneFrame;
typedef struct {
    double timer;
    Uint8 alpha;
    bool fading_in;
} FadeState;

enum direction { UP, DOWN, LEFT, RIGHT };
enum game_states { TITLE_SCREEN, CUTSCENE, OPEN_WORLD, BATTLE_SCREEN, DEATH_SCREEN, FINAL_SCREEN };
enum player_states { IDLE, MOVABLE, DIALOGUE, PAUSE, DEAD, IN_BATTLE };
enum characters { MENEGHETTI, MENEGHETTI_ANGRY, MENEGHETTI_SAD, PYTHON, NONE };
enum battle_buttons { FIGHT, ACT, ITEM, LEAVE };
enum battle_states { ON_MENU, ON_FIGHT, ON_ACT, ON_ITEM, ON_LEAVE };
enum battle_turns { CHOICE_TURN, ATTACK_TURN, SOUL_TURN, ACT_TURN };

bool sdl_initialize(Game *game);
// bool load_media(Game *game);
void object_cleanup(SDL_Texture *obj);
void game_cleanup(Game *game, int exit_status);
SDL_Texture *create_texture(SDL_Renderer *render, const char *dir);
SDL_Texture *create_txt(SDL_Renderer *render, const char *utf8_char, TTF_Font *font, SDL_Color color);
SDL_Texture *animate_sprite(Animation *anim, double dt, double cooldown, AnimState *state, bool blink);
void create_dialogue(Character *player, SDL_Renderer *render, Text *text, int *player_state, int *game_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Mix_Chunk **sound, bool bubble);
void reset_dialogue(Text *text);
void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], double *anim_timer, double anim_interval, Mix_Chunk **sound);
bool rects_intersect(SDL_Rect *a, SDL_Rect *b);
bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count);
static int surface_to_sound_index(int surface_index);
static int detect_surface(SDL_Rect *player, SDL_Rect surfaces[], int surface_count);
int renderitem_cmp(const void *pa, const void *pb);
void update_reflection(Character *original, Character* reflection, Animation *animation);
void organize_items(Prop *text_items);
int randint(int min, int max);
int choice(int count, ...);
void python_attacks(SDL_Renderer *render, Prop *soul, SDL_Rect battle_box, int *player_health, int damage, int attack_index, bool *ivulnerable, Prop **props, double dt, double turn_timer, bool clear);

static int utf8_charlen(const char *s);
static int utf8_copy_char(const char *s, char *out);

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    (void) argc;
    (void) argv;

    int game_state = CUTSCENE; // Lembrar de deixar CUTSCENE aqui na versão final.
    int battle_state = ON_MENU;
    FadeState open_world_fade = {0.0, 255, true};
    FadeState end_scene_fade = {0.0, 0, true};

    double arrival_timer = 0.0;
    double car_animation_timer = 0.0;
    bool delay_started = false;
    float parallax_factor = 0.5f;

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

    Animation anim_pack_reflex[DIR_COUNT];
    anim_pack_reflex[UP].count = 3;
    anim_pack_reflex[UP].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[UP].count);
    anim_pack_reflex[UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back.png");
    anim_pack_reflex[UP].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-1.png");
    anim_pack_reflex[UP].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-2.png");
    anim_pack_reflex[DOWN].count = 3;
    anim_pack_reflex[DOWN].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[DOWN].count);
    anim_pack_reflex[DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front.png");
    anim_pack_reflex[DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-1.png");
    anim_pack_reflex[DOWN].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-2.png");
    anim_pack_reflex[LEFT].count = 2;
    anim_pack_reflex[LEFT].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[LEFT].count);
    anim_pack_reflex[LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left.png");
    anim_pack_reflex[LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left-1.png");
    anim_pack_reflex[RIGHT].count = 2;
    anim_pack_reflex[RIGHT].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[RIGHT].count);
    anim_pack_reflex[RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right.png");
    anim_pack_reflex[RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right-1.png");
    for (int i = 0; i < DIR_COUNT; i++) {
        for (int n = 0; n < anim_pack_reflex[i].count; n++) {
            if (!anim_pack_reflex[i].frames[n]) {
                fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
                return 1;
            }
            else {
                SDL_SetTextureAlphaMod(anim_pack_reflex[i].frames[n], 70);
            }
        }
    }

    Animation meneghetti_dialogue[3];
    meneghetti_dialogue[0].count = 2;
    meneghetti_dialogue[0].frames = malloc(sizeof(SDL_Texture*) * meneghetti_dialogue[0].count);
    meneghetti_dialogue[0].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-1.png");
    meneghetti_dialogue[0].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-2.png");
    meneghetti_dialogue[1].count = 2;
    meneghetti_dialogue[1].frames = malloc(sizeof(SDL_Texture*) * meneghetti_dialogue[1].count);
    meneghetti_dialogue[1].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-1.png");
    meneghetti_dialogue[1].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-2.png");
    meneghetti_dialogue[2].count = 2;
    meneghetti_dialogue[2].frames = malloc(sizeof(SDL_Texture*) * meneghetti_dialogue[2].count);
    meneghetti_dialogue[2].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-sad-1.png");
    meneghetti_dialogue[2].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-sad-2.png");
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

    Animation mr_python_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/characters/mr-python.png"), create_texture(game.renderer, "assets/sprites/characters/mr-python-1.png")},
        .count = 2
    };

    TTF_Font* title_text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", 24);
    SDL_Color title_text_color = {101, 107, 117, 255};
    Animation title_text_anim = {
        .frames = (SDL_Texture*[]){create_txt(game.renderer, "APERTE ENTER PARA COMEÇAR", title_text_font, title_text_color), NULL},
        .count = 2
    };

    Animation lake_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/lake-1.png"), create_texture(game.renderer, "assets/sprites/scenario/lake-2.png"), create_texture(game.renderer, "assets/sprites/scenario/lake-3.png")},
        .count = 3
    };

    Animation ocean_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/ocean-1.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-2.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-3.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-4.png")},
        .count = 4
    };

    Animation sky_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/sky-1.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-2.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-3.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-4.png")},
        .count = 4
    };

    Animation sun_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/sun-1.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-2.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-3.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-4.png")},
        .count = 4
    };

    Animation soul_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/soul.png"), NULL},
        .count = 2
    };

    Animation bar_attack_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/bar-attack-2.png"), create_texture(game.renderer, "assets/sprites/battle/bar-attack-1.png")},
        .count = 2
    };

    Animation slash_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/slash-1.png"), create_texture(game.renderer, "assets/sprites/battle/slash-2.png"), create_texture(game.renderer, "assets/sprites/battle/slash-3.png"), create_texture(game.renderer, "assets/sprites/battle/slash-4.png"), create_texture(game.renderer, "assets/sprites/battle/slash-5.png"), create_texture(game.renderer, "assets/sprites/battle/slash-6.png")},
        .count = 6
    };

    Animation python_head_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-head-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-head-2.png")},
        .count = 2
    };

    Animation python_arms_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-arms.png"), create_texture(game.renderer, "assets/sprites/battle/python-arms-hurt.png")},
        .count = 2
    };
    
    Animation python_legs_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-legs.png"), create_texture(game.renderer, "assets/sprites/battle/python-legs-hurt.png")},
        .count = 2
    };

    Animation python_mother_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-2.png")},
        .count = 2
    };

    Animation python_baby_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-baby-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-baby-2.png")},
        .count = 2
    };

    // OBJETOS:
    Character meneghetti = {
        .texture = anim_pack[DOWN].frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32},
        .sprite_vel = 100.0f, // Deve ser par.
        .keystate = SDL_GetKeyboardState(NULL),
        .interact_collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25},
        .health = 20,
        .strength = 20,
        .facing = DOWN,
        .counters = {0, 0, 0, 0}
    };

    Character scenario = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/scenario.png"),
        .collision = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };
    if (!scenario.texture) {
        fprintf(stderr, "Error loading scenario: %s\n", SDL_GetError());
        return 1;
    }

    Character meneghetti_civic = {
        .texture = create_texture(game.renderer, "assets/sprites/characters/meneghetti-civic-left.png"),
        .collision = {scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42}
    };
    if (!meneghetti_civic.texture) {
        fprintf(stderr, "Error loading scenario: %s\n", SDL_GetError());
        return 1;
    }

    Character mr_python_head = {
        .texture = python_head_animation.frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 102, 25, 204, 204},
        .health = 200,
    };

    Character meneghetti_reflection = {
        .texture = anim_pack_reflex[DOWN].frames[0],
        .facing = DOWN,
        .counters = {0, 0, 0, 0}
    };

    // PROPS:
    Prop mr_python_torso = {
        .current = create_texture(game.renderer, "assets/sprites/battle/python-torso.png"),
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h}
    };

    Prop mr_python_arms = {
        .current = python_arms_animation.frames[0],
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h}
    };

    Prop mr_python_legs = {
        .current = python_legs_animation.frames[0],
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h}
    };

    Prop slash = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = slash_animation.frames[0],
        .collision = {mr_python_torso.collision.x + (mr_python_torso.collision.w / 2) + 16, mr_python_torso.collision.y + 32, 32, 164}
    };

    Prop title = {
        .current = create_texture(game.renderer, "assets/sprites/hud/logo-c-tale.png"),
        .collision.x = (SCREEN_WIDTH / 2) - 290,
        .collision.y = (SCREEN_HEIGHT / 2) - 32,
        .collision.w = 580,
        .collision.h = 63
    };

    Prop title_text = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = create_txt(game.renderer, "APERTE ENTER PARA COMEÇAR", title_text_font, title_text_color),
    };
    int title_text_width, title_text_height;
    SDL_QueryTexture(title_text.current, NULL, NULL, &title_text_width, &title_text_height);
    title_text.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - (title_text_width / 2), SCREEN_HEIGHT - 100, title_text_width, title_text_height};

    Prop soul = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = soul_animation.frames[0]
    };
    if (game_state == BATTLE_SCREEN) {
        soul.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 10, 20, 20};
    }

    Prop soul_shattered = {
        .current = create_texture(game.renderer, "assets/sprites/battle/soul-broken.png")
    };

    Prop mr_python = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = mr_python_animation.frames[0]
    };

    Prop python_van = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/python-van.png")
    };

    Prop civic = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/civic-left.png")
    };

    Prop palm_left = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/palm-head-left.png")
    };

    Prop palm_right = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/palm-head-right.png")
    };

    Prop lake = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = lake_animation.frames[0]
    };

    Prop ocean = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = ocean_animation.frames[0]
    };
    
    Prop sky = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = sky_animation.frames[0]
    };

    Prop mountains = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/mountains.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };

    Prop sun = {
        .current = sun_animation.frames[0]
    };

    Prop clouds = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/clouds.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, 155}
    };
    SDL_SetTextureAlphaMod(clouds.current, 200);
    SDL_Rect clouds_clone = {clouds.collision.x - clouds.collision.w, 0, SCREEN_WIDTH * 2, 155};

    Prop mountains_back = {
        .current = create_texture(game.renderer, "assets/sprites/scenario/mountains-back.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };

    SDL_Texture* fight_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-fight.png"), create_texture(game.renderer, "assets/sprites/hud/button-fight-select.png")};
    Prop button_fight = {
        .current = fight_b_textures[1],
        .collision = {26, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* act_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-act.png"), create_texture(game.renderer, "assets/sprites/hud/button-act-select.png")};
    Prop button_act = {
        .current = act_b_textures[0],
        .collision = {button_fight.collision.x + button_fight.collision.w + 26, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* item_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-item.png"), create_texture(game.renderer, "assets/sprites/hud/button-item-select.png")};
    Prop button_item = {
        .current = item_b_textures[0],
        .collision = {button_act.collision.x + button_act.collision.w + 25, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* leave_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-leave.png"), create_texture(game.renderer, "assets/sprites/hud/button-leave-select.png")};
    Prop button_leave = {
        .current = leave_b_textures[0],
        .collision = {button_item.collision.x + button_item.collision.w + 25, SCREEN_HEIGHT - 68, 128, 48}
    };

    // PROPS DA BATALHA:
    int battle_text_width, battle_text_height;
    TTF_Font* battle_text_font = TTF_OpenFont("assets/fonts/PixelOperatorSC-Bold.ttf", 24);
    SDL_Color battle_text_color = {255, 255, 255, 255};
    Prop battle_name = {
        .current = create_txt(game.renderer, "MENEGHETTI", battle_text_font, battle_text_color),
    };
    SDL_QueryTexture(battle_name.current, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_name.collision = (SDL_Rect){button_fight.collision.x + 6, button_fight.collision.y - battle_text_height - 8, battle_text_width,battle_text_height};

    Prop battle_hp = {
        .current = create_txt(game.renderer, "HP", battle_text_font, battle_text_color),
    };
    SDL_QueryTexture(battle_hp.current, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_hp.collision = (SDL_Rect){button_act.collision.x + 35, button_fight.collision.y - battle_text_height - 8, battle_text_width, battle_text_height};

    Prop battle_hp_amount = {
        .current = create_txt(game.renderer, "20/20", battle_text_font, battle_text_color),
    };
    SDL_QueryTexture(battle_hp_amount.current, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_hp_amount.collision = (SDL_Rect){button_act.collision.x + 140, button_fight.collision.y - battle_text_height - 8, battle_text_width, battle_text_height};

    Prop food_amount_text = {
        .current = create_txt(game.renderer, "4x", battle_text_font, battle_text_color),
    };
    SDL_QueryTexture(food_amount_text.current, NULL, NULL, &battle_text_width, &battle_text_height);
    food_amount_text.collision = (SDL_Rect){0, 0, battle_text_width, battle_text_height};

    Prop text_attack_act = {
        .current = create_txt(game.renderer, "* Mr. Python", title_text_font, battle_text_color),
    };
    SDL_QueryTexture(text_attack_act.current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_attack_act.collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};

    Prop text_item = {
        .current = create_txt(game.renderer, "* Picanha", title_text_font, battle_text_color)
    };
    SDL_QueryTexture(text_item.current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_item.collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};

    Prop text_act[3];
    text_act[0].current = create_txt(game.renderer, "* Examinar", title_text_font, battle_text_color);
    SDL_QueryTexture(text_act[0].current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[0].collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};
    text_act[1].current = create_txt(game.renderer, "* Insultar", title_text_font, battle_text_color);
    SDL_QueryTexture(text_act[1].current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[1].collision = (SDL_Rect){69, text_act[0].collision.y + text_act[0].collision.h + 10, battle_text_width, battle_text_height};
    text_act[2].current = create_txt(game.renderer, "* Explicar", title_text_font, battle_text_color);
    SDL_QueryTexture(text_act[2].current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[2].collision = (SDL_Rect){text_act[0].collision.x + text_act[0].collision.w + 100, text_act[0].collision.y, battle_text_width, battle_text_height};

    Prop text_leave[2];
    text_leave[0].current = create_txt(game.renderer, "* Poupar", title_text_font, battle_text_color);
    SDL_QueryTexture(text_leave[0].current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_leave[0].collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};
    text_leave[1].current = create_txt(game.renderer, "* Fugir", title_text_font, battle_text_color);
    SDL_QueryTexture(text_leave[1].current, NULL, NULL, &battle_text_width, &battle_text_height);
    text_leave[1].collision = (SDL_Rect){69, text_leave[0].collision.y + text_leave[0].collision.h + 10, battle_text_width, battle_text_height};

    Prop bar_target = {
        .current = create_texture(game.renderer, "assets/sprites/battle/bar-target.png"),
        .collision = {25, (SCREEN_HEIGHT / 2) + 5, SCREEN_WIDTH - 50, 122}
    };
    Prop bar_attack = {
        .anim_state.counter = 0,
        .anim_state.timer = 0.0,
        .current = bar_attack_animation.frames[0],
        .collision = {bar_target.collision.x + 20, bar_target.collision.y + 2, 14, bar_target.collision.h - 4}
    };

    int attack_widths, attack_heights;
    Prop command_rain[6];
    command_rain[0].current = create_texture(game.renderer, "assets/sprites/battle/if.png");
    SDL_QueryTexture(command_rain[0].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    command_rain[1].current = create_texture(game.renderer, "assets/sprites/battle/else.png");
    SDL_QueryTexture(command_rain[1].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    command_rain[2].current = create_texture(game.renderer, "assets/sprites/battle/elif.png");
    SDL_QueryTexture(command_rain[2].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    command_rain[3].current = create_texture(game.renderer, "assets/sprites/battle/input.png");
    SDL_QueryTexture(command_rain[3].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    command_rain[4].current = create_texture(game.renderer, "assets/sprites/battle/print.png");
    SDL_QueryTexture(command_rain[4].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[4].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    command_rain[5].current = create_texture(game.renderer, "assets/sprites/battle/in.png");
    SDL_QueryTexture(command_rain[5].current, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[5].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

    Prop parenthesis_enclosure[6];
    parenthesis_enclosure[0].current = create_texture(game.renderer, "assets/sprites/battle/brackets-1.png");
    SDL_QueryTexture(parenthesis_enclosure[0].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[1].current = create_texture(game.renderer, "assets/sprites/battle/brackets-2.png");
    SDL_QueryTexture(parenthesis_enclosure[1].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[2].current = create_texture(game.renderer, "assets/sprites/battle/key-1.png");
    SDL_QueryTexture(parenthesis_enclosure[2].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[3].current = create_texture(game.renderer, "assets/sprites/battle/key-2.png");
    SDL_QueryTexture(parenthesis_enclosure[3].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[4].current = create_texture(game.renderer, "assets/sprites/battle/parenthesis-1.png");
    SDL_QueryTexture(parenthesis_enclosure[4].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[4].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[5].current = create_texture(game.renderer, "assets/sprites/battle/parenthesis-2.png");
    SDL_QueryTexture(parenthesis_enclosure[5].current, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[5].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

    Prop python_mother[4];
    python_mother[0].current = python_mother_animation.frames[0];
    SDL_QueryTexture(python_mother[0].current, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    python_mother[1].current = python_mother_animation.frames[1];
    SDL_QueryTexture(python_mother[1].current, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    python_mother[2].current = python_baby_animation.frames[0];
    SDL_QueryTexture(python_mother[2].current, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
    python_mother[3].current = python_baby_animation.frames[1];
    SDL_QueryTexture(python_mother[3].current, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

    Prop *python_props[] = {command_rain, parenthesis_enclosure, python_mother};

    SDL_Texture* damage_numbers[] = {create_texture(game.renderer, "assets/sprites/battle/number-10.png"), create_texture(game.renderer, "assets/sprites/battle/number-20.png"), create_texture(game.renderer, "assets/sprites/battle/number-30.png"), create_texture(game.renderer, "assets/sprites/battle/number-40.png")};
    Prop damage;

    // SONS:
    Mix_Chunk* cutscene_music = Mix_LoadWAV("assets/sounds/soundtracks/the_story_of_a_hero.wav");
    Mix_Chunk* battle_music = Mix_LoadWAV("assets/sounds/soundtracks/battle_against_abstraction.wav");

    Mix_Chunk* ambience = Mix_LoadWAV("assets/sounds/sound_effects/in-game/ambient_sound.wav");
    if (!ambience) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(ambience, 35);

    Mix_Chunk* walking_sounds[] = {Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_grass.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_concrete.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_sand.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_bridge.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_wood.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/walking_dirt.wav")};
    for (int i = 0; i < 5; i++) {
        if (!walking_sounds[i]) {
            fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
            return 1;
        }
    }
    Mix_Volume(0, 35); // Volume do canal dos efeitos sonoros.

    Mix_Chunk* dialogue_voices[] = {Mix_LoadWAV("assets/sounds/sound_effects/in-game/meneghetti_voice.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/mr_python_voice.wav"), Mix_LoadWAV("assets/sounds/sound_effects/in-game/text_sound.wav")};
    for (int i = 0; i < 2; i++) {
        if (!dialogue_voices[i]) {
            fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
            return 1;
        }
    }

    Mix_Chunk* civic_engine = Mix_LoadWAV("assets/sounds/sound_effects/in-game/car_engine.wav");
    Mix_Chunk* civic_brake = Mix_LoadWAV("assets/sounds/sound_effects/in-game/car_brake.wav");
    Mix_Chunk* civic_door = Mix_LoadWAV("assets/sounds/sound_effects/in-game/car_door.wav");

    Mix_Chunk* title_sound = Mix_LoadWAV("assets/sounds/sound_effects/in-game/logo_sound.wav");

    Mix_Chunk* battle_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/battle_appears.wav");
    Mix_Chunk* move_button = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/move_selection.wav");
    Mix_Chunk* click_button = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/select_sound.wav");
    Mix_Chunk* slash_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/slash.wav");
    Mix_Chunk* enemy_hit_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/enemy_hit.wav");
    Mix_Chunk* eat_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/heal_sound.wav");

    Mix_Chunk* soul_break_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/soul_shatter.wav");

    // BASES DE TEXTO:
    Text py_dialogue = {
        .writings = {"* Há quanto tempo, Meneghetti.", "* Mr. Python...", "* Você veio até aqui batalhar contra mim?", "* Lembra o que aconteceu da última vez, não é?", "* Você e as outras linguagens de baixo nível nem me arranharam. Foi realmente estúpido.", "* Não vou cometer os mesmos erros do passado...", "* Você vai pagar pelo que fez com eles.", "* As linguagens de baixo nível ainda não morreram.", "* Eu ainda estou aqui para acabar com você.", "* Que peninha... Deve ser tão triste ser o último que restou.", "* Eu entendo a sua frustração.", "* Vamos acabar com isso para que você se junte a eles logo.", "* Venha, Mr. Python."},
        .on_frame = {PYTHON, MENEGHETTI, PYTHON, PYTHON, PYTHON, MENEGHETTI_SAD, MENEGHETTI_ANGRY, MENEGHETTI, MENEGHETTI_ANGRY, PYTHON, PYTHON, PYTHON, MENEGHETTI_ANGRY},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text py_dialogue_ad = {
        .writings = {"* Hm? Você conseguiu voltar?", "* Você é realmente duro na queda, Meneghetti. Devo admitir.", "* Eu ainda não desisti. Não pense que vai ser fácil.", "* Vamos ver se você vai ter a mesma sorte desta vez."},
        .on_frame = {PYTHON, PYTHON, MENEGHETTI_ANGRY, PYTHON},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue_ad.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text py_dialogue_ad_2 = {
        .writings = {"* Que insistência. Por que não desiste logo?", "* Não enquanto eu não acabar com você.", "* Hahahah. Não precisa ser tão agressivo."},
        .on_frame = {PYTHON, MENEGHETTI_ANGRY, PYTHON},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue_ad_2.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text py_dialogue_ad_3 = {
        .writings = {"* Acho que tá um pouco difícil para você. Quer que eu diminua a dificuldade?", "* Cala a boca.", "* Desculpa, pessoal, eu tentei. Vamos para mais um round então."},
        .on_frame = {PYTHON, MENEGHETTI, PYTHON},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue_ad_3.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text py_dialogue_ad_4 = {
        .writings = {"* ...", "* Vamos logo com isso."},
        .on_frame = {MENEGHETTI, PYTHON},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!py_dialogue_ad_4.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text van_dialogue = {
        .writings = {"* Então este é o Python-móvel...", "* Agora tenho certeza de que meu inimigo está aqui..."},
        .on_frame = {MENEGHETTI, MENEGHETTI_ANGRY},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    if (!van_dialogue.text_font) {
        fprintf(stderr, "Error loading font: %s", TTF_GetError());
        return 1;
    }

    Text lake_dialogue = {
        .writings = {"* O lago com animação te faz pensar sobre os esforços do criador deste universo.", "* Isso te enche de determinação.", "* Se fosse programado em Python não daria pra fazer isso."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE, NONE, MENEGHETTI},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text arrival_dialogue = {
        .writings = {"* Meu radar-C detectou locomoções de alto nível por esta área.", "* Hora de acabar com isso de uma vez por todas."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {MENEGHETTI, MENEGHETTI},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text end_dialogue = {
        .writings = {"* Como... Como que isso foi acontecer?", "* Não faz sentido... Nós tínhamos ganhado essa luta.", "* EU já havia ganhado.", "* ...", "* Esse não é o fim, Meneghetti.", "* Por agora, você venceu. Mas um dia...", "* Um dia, as linguagens de baixo nível serão esquecidas.", "* E esse será o dia de sua ruína, e do meu triunfo.", "* Após anos de reinado das linguagens de alto nível...", "* A luz que um dia havia sumido dos programadores finalmente voltou a brilhar.", "* Um raio de esperança e um futuro próspero agora poderiam ser contemplados.", "* Tudo isso graças à ele..."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, NONE, NONE, NONE, NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_1 = {
        .writings = {"Na época de ouro da computação, o mundo vivia em harmonia com diversas linguagens de programação."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    
    Text cutscene_2 = {
        .writings = {"Porém, com os avanços tecnológicos, surgiu dependência e abstração na vida dos programadores."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_3 = {
        .writings = {"No fim, restaram mínimos usuários de linguagens de baixo nível, o mundo fora tomado pela praticidade. Mas ainda havia resistência."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_4 = {
        .writings = {"Para trazer a luz para o mundo novamente, um dos heróis restantes lutará contra todas as abstrações e seu maior inimigo..."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text fight_start_txt = {
        .writings = {"* Mr. Python bloqueia o seu caminho."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text fight_generic_txt = {
        .writings = {"* Mr. Python aguarda o seu próximo movimento."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text fight_leave_txt = {
        .writings = {"* Esta é uma batalha em que você não cogita fugir."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text fight_spare_txt = {
        .writings = {"* A palavra 'perdão' não existe no seu vocabulário neste momento."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text fight_act_txt = {
        .writings = {"* Mr. Python - 2 ATQ, ? DEF |* O seu pior inimigo."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text insult_txt = {
        .writings = {"* Você insulta a tipagem dinâmica. |* Mr. Python aumenta a sua própria variável de força."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text insult_generic_txt = {
        .writings = {"* Você lembra do último turno... |* Você decide ficar calado."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text explain_txt = {
        .writings = {"* Você explica ponteiros para Mr. Python. |* Ele enfraquece ao ouvir algo tão rudimentar."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text explain_generic_txt = {
        .writings = {"* Você tenta explicar algo de baixo nível, mas Mr. Python dá de costas. |* Que rude!"},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text picanha_txt = {
        .writings = {"* Você comeu PICANHA. |* Você recuperou 20 de HP!"},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text no_food_txt = {
        .writings = {"* Não sobrou mais nada comestível em seus bolsos."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", DIALOGUE_FONT_SIZE),
        .on_frame = {NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text bubble_speech_1 = {
        .writings = {"A abstração já venceu há muito tempo."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", 14),
        .on_frame = {NONE},
        .text_color = {0, 0, 0, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text bubble_speech_2 = {
        .writings = {"As linguagens de baixo nível já estão ultrapassadas."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", 14),
        .on_frame = {NONE},
        .text_color = {0, 0, 0, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text bubble_speech_3 = {
        .writings = {"Te darei um final digno."},
        .text_font = TTF_OpenFont("assets/fonts/PixelOperator-Bold.ttf", 14),
        .on_frame = {NONE},
        .text_color = {0, 0, 0, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    // FRAMES DA CUTSCENE:
    CutsceneFrame frame_1 = {
        .text = &cutscene_1,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-1.png"),
        .duration = 10.0
    };
    CutsceneFrame frame_2 = {
        .text = &cutscene_2,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-2.png"),
        .duration = 10.0
    };
    CutsceneFrame frame_3 = {
        .text = &cutscene_3,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-1.2.png"),
        .duration = 12.0
    };
    CutsceneFrame frame_4 = {
        .text = &cutscene_4,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-4.png"),
        .duration = 13.5
    };

    CutsceneFrame cutscene[] = {frame_1, frame_2, frame_3, frame_4};
    int cutscene_amount = sizeof(cutscene) / sizeof(cutscene[0]);

    Uint32 last_ticks = SDL_GetTicks();
    double anim_timer = 0.0;
    const double anim_interval = 0.12;

    double cloud_timer = 0.0;
    bool interaction_request = false;
    bool ambience_begun = false;
    bool meneghetti_arrived = false;
    bool python_dialogue_finished = false;

    int last_health = meneghetti.health;

    // VARIÁVEIS DE CONTROLE
    int food_amount = 4;
    int selected_button = FIGHT;
    int turn = CHOICE_TURN;
    int menu_pos = 0;
    int base_py_damage = 2;
    int current_py_damage = base_py_damage;
    double senoidal_timer = 0.0;
    double counter = 0.0;
    bool battle_ready = false;
    bool on_dialogue = false;
    bool first_dialogue = false;
    SDL_Rect animated_box;
    bool animated_box_inited = false;
    double animated_shrink_timer = 0.0;
    const double animated_shrink_duration = 0.8;
    bool enemy_attack_selected = false;
    bool tried_to_attack = false;
    double blink_timer = 0.0;
    bool has_played_slash = false;
    bool has_played_hit = false;
    bool soul_ivulnerable = false;
    double ivulnerability_counter = 0.0;
    double turn_timer = 0.0;
    bool should_expand_back = false;
    bool first_insult = true;
    bool first_explain = true;
    bool has_played_eat = false;
    double death_counter  = 0.0;
    bool has_played_break = false;
    bool python_dead = false;
    
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

        if (game_state == CUTSCENE) {
            static double pre_title_timer = 0.0;
            static bool pre_title = true;
            
            static bool music = false;
            static int cutscene_index = 0;
            static double cutscene_timer = 0.0;
            static FadeState fade = {0, 0, true};
            static bool last_frame_extend = false;

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 0);

            if (pre_title) {
                pre_title_timer += dt;
                
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, title.current, NULL, &title.collision);

                if (pre_title_timer >= 5) {
                    pre_title = false;
                }
            }
            else {
                if (!Mix_Playing(3) && !music) {
                    Mix_PlayChannel(3, cutscene_music, 0);
                    music = true;
                }
                CutsceneFrame *current_frame = &cutscene[cutscene_index];
                cutscene_timer += dt;

                if (fade.fading_in) {
                    fade.timer += dt;
                    fade.alpha = (Uint8)((fade.timer / 1.0) * 255);
                    if (fade.timer >= 1.0) {
                        fade.alpha = 255;
                        fade.fading_in = false;
                        fade.timer = 0.0;
                    }
                }
                else if (cutscene_timer >= current_frame->duration - 1.0) {
                    fade.timer += dt;
                    fade.alpha = 255 - (Uint8)((fade.timer / 1.0) * 255);
                    if (fade.timer >= 1.0) {
                        fade.alpha = 0;
                    }
                }

                if (cutscene_index == cutscene_amount - 1 && cutscene_timer >= current_frame->duration - 3.0 && !last_frame_extend) {
                    last_frame_extend = true;
                    fade.timer = 0.0;
                }

                if (last_frame_extend) {
                    fade.timer += dt;
                    fade.alpha = 255 - (Uint8)((fade.timer / 5.0) * 255);
                    if (fade.timer >= 5.0) {
                        fade.alpha = 0;
                    }
                }

                SDL_SetTextureAlphaMod(current_frame->image, fade.alpha);
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, current_frame->image, NULL, NULL);

                if (current_frame->text) {
                    create_dialogue(&meneghetti, game.renderer, current_frame->text, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                }

                if (cutscene_timer >= current_frame->duration + 0.5 && !last_frame_extend) {
                    cutscene_timer = 0.0;
                    cutscene_index++;
                    fade.fading_in = true;
                    fade.timer = 0.0;
                    fade.alpha = 0;

                    if (cutscene_index >= cutscene_amount) {
                        cutscene_index = cutscene_amount - 1;
                        last_frame_extend = true;
                        fade.timer = 0.0;
                    }
                }

                if (interaction_request) {
                    game_state = TITLE_SCREEN;
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                }
                if (last_frame_extend && fade.timer >= 3.0) {
                    game_state = TITLE_SCREEN;
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                }
            }
            interaction_request = false;
        }

        if (game_state == TITLE_SCREEN) {
            const Uint8 *keys = meneghetti.keystate ? meneghetti.keystate : SDL_GetKeyboardState(NULL);
            static bool sound_has_played = false;

            SDL_RenderClear(game.renderer);
            SDL_RenderCopy(game.renderer, title.current, NULL, &title.collision);

            if (!sound_has_played) {
                Mix_PlayChannel(3, title_sound, 0);
                sound_has_played = true;
            }
            if (!Mix_Playing(3)) {
                title_text.current = animate_sprite(&title_text_anim, dt, 0.7, &title_text.anim_state, false);
                SDL_RenderCopy(game.renderer, title_text.current, NULL, &title_text.collision);

                if (keys[SDL_SCANCODE_RETURN]) {
                    sound_has_played = false;
                    player_state = IDLE;
                    game_state = OPEN_WORLD;
                }
            }
        }

        if (game_state == OPEN_WORLD) {
            if (open_world_fade.fading_in) {
                open_world_fade.timer += dt;
                open_world_fade.alpha = 255 - (Uint8)((open_world_fade.timer / 5.0) * 255);

                if (open_world_fade.timer >= 5.0) {
                    open_world_fade.alpha = 0;
                    open_world_fade.fading_in = false;
                }
            }
            if (!ambience_begun) {
                Mix_PlayChannel(2, ambience, -1);
                ambience_begun = true;
            }

            if (clouds.collision.x <= scenario.collision.x + scenario.collision.w) {
                cloud_timer += dt;
                if (cloud_timer >= 0.2) {
                    clouds.collision.x++;
                    clouds_clone.x++;
                    cloud_timer = 0.0;
                }
            }
            else {
                clouds.collision.x = scenario.collision.x;
                clouds_clone.x = clouds.collision.x - clouds.collision.w;
            }

            mountains.collision.x = scenario.collision.x * parallax_factor;
            mountains.collision.y = scenario.collision.y * parallax_factor;
            mountains_back.collision.x = scenario.collision.x * (parallax_factor / 2);
            mountains_back.collision.y = scenario.collision.y * (parallax_factor / 2);

            // PROPS:
            sky.collision = (SDL_Rect){scenario.collision.x * (parallax_factor / 4), scenario.collision.y * (parallax_factor / 4), scenario.collision.w, scenario.collision.h};
            sun.collision = (SDL_Rect){scenario.collision.x * (parallax_factor / 4), scenario.collision.y * (parallax_factor / 4), scenario.collision.w, scenario.collision.h};
            soul.collision = (SDL_Rect){meneghetti.collision.x, meneghetti.collision.y + 8, 20, 20};
            lake.collision = (SDL_Rect){scenario.collision.x, scenario.collision.y, scenario.collision.w, scenario.collision.h};
            ocean.collision = (SDL_Rect){scenario.collision.x * (parallax_factor * 1.4), scenario.collision.y * (parallax_factor * 1.4), scenario.collision.w, scenario.collision.h};
            mr_python.collision = (SDL_Rect){scenario.collision.x + 620, scenario.collision.y + 153, 39, 64};
            python_van.collision = (SDL_Rect){scenario.collision.x + 758, scenario.collision.y + 592, 64, 33};
            if (meneghetti_arrived)
                civic.collision = (SDL_Rect){scenario.collision.x + 250, scenario.collision.y + 749, 65, 25};

            // SUPERFÍCIES:
            SDL_Rect surfaces[SURFACE_QUANTITY];
            surfaces[0] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 569, 611, 135}; // Grama esquerda.
            surfaces[1] = (SDL_Rect){scenario.collision.x + 669, scenario.collision.y + 569, 611, 135}; // Grama direita.
            surfaces[2] = (SDL_Rect){scenario.collision.x + 653, scenario.collision.y + 590, 16, 49}; // Grama restante direita.
            surfaces[3] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 704, scenario.collision.w, 41}; // Calçada.
            surfaces[4] = (SDL_Rect){scenario.collision.x + 981, scenario.collision.y + 745, 64, 72}; // Faixa de pedestres.
            surfaces[5] = (SDL_Rect){scenario.collision.x + 981, scenario.collision.y + 817, 64, 40}; // Pedras.
            surfaces[6] = (SDL_Rect){scenario.collision.x + 981, scenario.collision.y + 857, 64, 33}; // Areia.
            surfaces[7] = (SDL_Rect){scenario.collision.x + 607, scenario.collision.y + 398, 66, 152}; // Ponte.
            surfaces[8] = (SDL_Rect){scenario.collision.x + 253, scenario.collision.y + 106, 774, 278}; // Píer.
            surfaces[9] = (SDL_Rect){scenario.collision.x + 607, scenario.collision.y + 550, 66, 19}; // Caminho de terra (topo).
            surfaces[10] = (SDL_Rect){scenario.collision.x + 611, scenario.collision.y + 569, 58, 21}; // Caminho de terra (superior).
            surfaces[11] = (SDL_Rect){scenario.collision.x + 611, scenario.collision.y + 590, 42, 49}; // Caminho de terra (meio).
            surfaces[12] = (SDL_Rect){scenario.collision.x + 611, scenario.collision.y + 639, 58, 65}; // Caminho de terra (inferior).

            // COLISÕES:
            SDL_Rect boxes[COLLISION_QUANTITY];
            boxes[0] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 739, 981, 221}; // Bloco inferior esquerdo.
            boxes[1] = (SDL_Rect){scenario.collision.x, scenario.collision.y + 384, 607, 185}; // Bloco superior esquerdo (ponte).
            boxes[2] = (SDL_Rect){scenario.collision.x, scenario.collision.y, 253, 384}; // Bloco ao topo esquerdo.
            boxes[3] = (SDL_Rect){scenario.collision.x + 253, scenario.collision.y, 774, 105}; // Bloco ao topo central.
            boxes[4] = (SDL_Rect){scenario.collision.x + 1027, scenario.collision.y, 253, 384}; // Bloco ao topo direito.
            boxes[5] = (SDL_Rect){scenario.collision.x + 673, scenario.collision.y + 384, 607, 185}; // Bloco superior direito (ponte).
            boxes[6] = (SDL_Rect){scenario.collision.x + 1045, scenario.collision.y + 739, 235, 221}; // Bloco inferior esquerdo.
            boxes[7] = (SDL_Rect){scenario.collision.x + 981, scenario.collision.y + 890, 64, 70}; // Bloco do rodapé (lago).
            boxes[8] = (SDL_Rect){scenario.collision.x + 627, scenario.collision.y + 207, 25, 10}; // Bloco do Mr. Python.
            boxes[9] = (SDL_Rect){scenario.collision.x + 758, scenario.collision.y + 616, 64, 10}; // Bloco da Python Van.
            boxes[10] = (SDL_Rect){scenario.collision.x + 596, scenario.collision.y + 377, 11, 7}; // Toco esquerdo da ponte.
            boxes[11] = (SDL_Rect){scenario.collision.x + 673, scenario.collision.y + 377, 11, 7}; // Toco direito da ponte.

            if (player_state == MOVABLE) {
                sprite_update(&scenario, &meneghetti, anim_pack, dt, boxes, surfaces, &anim_timer, anim_interval, walking_sounds);
            }
            if (interaction_request) {
                if (rects_intersect(&meneghetti.interact_collision, &boxes[8]))
                    player_state = DIALOGUE;

                if (rects_intersect(&meneghetti.interact_collision, &boxes[9]))
                    player_state = DIALOGUE;
                
                if (rects_intersect(&meneghetti.interact_collision, &boxes[7]))
                    player_state = DIALOGUE;

                interaction_request = false;
            }

            update_reflection(&meneghetti, &meneghetti_reflection, anim_pack_reflex);

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer); 

            SDL_RenderCopy(game.renderer, sky.current, NULL, &sky.collision);
            SDL_RenderCopy(game.renderer, sun.current, NULL, &sun.collision);
            SDL_RenderCopy(game.renderer, clouds.current, NULL, &clouds.collision);
            SDL_RenderCopy(game.renderer, clouds.current, NULL, &clouds_clone);
            SDL_RenderCopy(game.renderer, mountains_back.current, NULL, &mountains_back.collision);
            SDL_RenderCopy(game.renderer, mountains.current, NULL, &mountains.collision);
            SDL_RenderCopy(game.renderer, ocean.current, NULL, &ocean.collision);
            SDL_RenderCopy(game.renderer, lake.current, NULL, &lake.collision);
            SDL_RenderCopyEx(game.renderer, meneghetti_reflection.texture, NULL, &meneghetti_reflection.collision, 0, NULL, SDL_FLIP_VERTICAL);
            SDL_RenderCopy(game.renderer, scenario.texture, NULL, &scenario.collision);

            mr_python.current = animate_sprite(&mr_python_animation, dt, 3.0, &mr_python.anim_state, true);
            lake.current = animate_sprite(&lake_animation, dt, 0.5, &lake.anim_state, false);
            ocean.current = animate_sprite(&ocean_animation, dt, 0.7, &ocean.anim_state, false);
            sky.current = animate_sprite(&sky_animation, dt, 0.8, &sky.anim_state, false);
            sun.current = animate_sprite(&sun_animation, dt, 0.5, &sun.anim_state, false);

            RenderItem items[2];
            int item_count = 0;

            items[item_count].texture = mr_python.current;
            items[item_count].collisions = &mr_python.collision;
            item_count ++;

            items[item_count].texture = python_van.current;
            items[item_count].collisions = &python_van.collision;
            item_count ++;
            
            if (meneghetti_arrived) {
                items[item_count].texture = civic.current;
                items[item_count].collisions = &civic.collision;
                item_count ++;

                items[item_count].texture = meneghetti.texture;
                items[item_count].collisions = &meneghetti.collision;
                item_count++;
            }

            qsort(items, item_count, sizeof(RenderItem), renderitem_cmp);

            for (int i = 0; i < item_count; i++) {
                if (items[i].texture && items[i].collisions) {
                    SDL_RenderCopy(game.renderer, items[i].texture, NULL, items[i].collisions);
                }
            }

            if (first_dialogue && player_state == DIALOGUE) {
                arrival_timer += dt;
                if (arrival_timer >= 1.5) {
                    create_dialogue(&meneghetti, game.renderer, &arrival_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                }
            }
            else if (first_dialogue && player_state == MOVABLE) {
                arrival_timer = 0.0;
                first_dialogue = false;
            }

            if (player_state == DIALOGUE) {
                if (rects_intersect (&meneghetti.interact_collision, &boxes[8])) {
                    create_dialogue(&meneghetti, game.renderer, &py_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                    python_dialogue_finished = true;
                }

                if (rects_intersect (&meneghetti.interact_collision, &boxes[9]))
                    create_dialogue(&meneghetti, game.renderer, &van_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                    
                if (rects_intersect (&meneghetti.interact_collision, &boxes[7]))
                    create_dialogue(&meneghetti, game.renderer, &lake_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
            }
            else if (python_dialogue_finished) {
                Mix_HaltChannel(2);
                player_state = IN_BATTLE;
                game_state = BATTLE_SCREEN;
            }

            if (!meneghetti_arrived) {
                palm_left.collision = (SDL_Rect){scenario.collision.x + 455, scenario.collision.y + 763, 73, 42};
                palm_right.collision = (SDL_Rect){scenario.collision.x + 531, scenario.collision.y + 750, 73, 42};
                car_animation_timer += dt;

                if (meneghetti_civic.collision.x > scenario.collision.x + 250) {
                    if (!Mix_Playing(3))
                        Mix_PlayChannel(3, civic_engine, 0);
                    
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    meneghetti_civic.collision.x -= 5;
                    meneghetti_civic.collision.y = (int)((scenario.collision.y + 731) + 2 * sin(car_animation_timer * 30.0)); 
                }
                else if (!delay_started) {
                    Mix_PlayChannel(3, civic_brake, 0);
                    
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    delay_started = true;
                    arrival_timer = 0.0;
                }
                else {
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    if (delay_started && !Mix_Playing(3)) {
                        arrival_timer += dt;
                        if (arrival_timer >= 2.0) {
                            Mix_PlayChannel(3, civic_door, 0);

                            delay_started = false;
                            meneghetti_arrived = true;
                            player_state = DIALOGUE;
                            first_dialogue = true;
                            arrival_timer = 0.0;
                        }
                    }
                }
                SDL_RenderCopy(game.renderer, palm_left.current, NULL, &palm_left.collision);
                SDL_RenderCopy(game.renderer, palm_right.current, NULL, &palm_right.collision);
            }
            if (open_world_fade.alpha > 0) {
                SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, open_world_fade.alpha);
                SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);

                SDL_Rect screen_fade = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
                SDL_RenderFillRect(game.renderer, &screen_fade);
            }
        }

        if (game_state == BATTLE_SCREEN) {
            const Uint8 *keys = meneghetti.keystate ? meneghetti.keystate : SDL_GetKeyboardState(NULL);
            Uint8 e_down = keys[SDL_SCANCODE_E];
            static Uint8 e_prev = 0;
            Uint8 e_just_pressed = (e_down && !e_prev) ? 1 : 0;

            if (meneghetti.health > 20) meneghetti.health = 20;
            if (meneghetti.health <= 0) {
                meneghetti.health = 20;
                player_state = DEAD;
            }
            
            SDL_Rect base_box = {20, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 40, 132};

            static char x_number[3];
            snprintf(x_number, sizeof(x_number), "%dx", food_amount);
            food_amount_text.current = create_txt(game.renderer, x_number, battle_text_font, battle_text_color);

            if (!animated_box_inited) {
                animated_box = base_box;
                animated_box_inited = true;
                animated_shrink_timer = 0.0;
            }

            SDL_Rect box_borders[] = {
                {animated_box.x, animated_box.y, animated_box.w, 5},
                {animated_box.x, animated_box.y, 5, animated_box.h},
                {animated_box.x, animated_box.y + animated_box.h - 5, animated_box.w, 5},
                {animated_box.x + animated_box.w - 5, animated_box.y, 5, animated_box.h}
            };
            SDL_Rect life_bar_background = {(SCREEN_WIDTH / 2) - 72, button_fight.collision.y - 30, 60, 20};
            SDL_Rect life_bar = {(SCREEN_WIDTH / 2) - 72, button_fight.collision.y - 30, meneghetti.health * 3, 20};
            SDL_Rect py_life_background = {(SCREEN_WIDTH / 2) - 100, 200, 200, 10};
            SDL_Rect py_life = {(SCREEN_WIDTH / 2) - 100, 200, 200, 10};

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            if (!battle_ready) {
                counter += dt;

                SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                if (counter <= 0.5) {
                    if (!Mix_Playing(3))
                        Mix_PlayChannel(3, battle_sound, 0);
                    soul.current = animate_sprite(&soul_animation, dt, 0.1, &soul.anim_state, false);
                }
                else {
                    soul.current = soul_animation.frames[0];
                    if (soul.collision.x != button_fight.collision.x + 30 || soul.collision.y != button_fight.collision.y + 30) {
                        if (abs(soul.collision.x - (button_fight.collision.x + 30)) <= 5)
                            soul.collision.x = button_fight.collision.x + 30;
                        else if (soul.collision.x < button_fight.collision.x + 30)
                            soul.collision.x += 5;
                        else if (soul.collision.x > button_fight.collision.x + 30)
                            soul.collision.x -= 5;
                        
                        if (abs(soul.collision.y - (button_fight.collision.y + 30)) <= 5)
                            soul.collision.y = button_fight.collision.y + 30;
                        else if (soul.collision.y > button_fight.collision.y + 30)
                            soul.collision.y -= 5;
                        else if (soul.collision.y < button_fight.collision.y + 30)
                            soul.collision.y += 5;
                    }
                    else {
                        soul.current = soul_animation.frames[0];
                        battle_ready = true;
                        counter = 0.0;
                    }
                }
            }
            else {
                if (!Mix_Playing(5)) {
                    Mix_PlayChannel(5, battle_music, 0);
                }
                counter += dt;
                senoidal_timer += dt;
                if (counter >= 0.4) counter = 0.2;

                if (meneghetti.health != last_health) {
                    char hp_string[6];
                    snprintf(hp_string, sizeof(hp_string), "%02d/20", meneghetti.health);

                    battle_hp_amount.current = create_txt(game.renderer, hp_string, battle_text_font, battle_text_color);

                    last_health = meneghetti.health;
                }

                SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
                SDL_RenderFillRect(game.renderer, &base_box);
                SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
                SDL_RenderFillRects(game.renderer, box_borders, 4);
                SDL_SetRenderDrawColor(game.renderer, 168, 24, 13, 255);
                SDL_RenderFillRect(game.renderer, &life_bar_background);
                SDL_SetRenderDrawColor(game.renderer, 204, 195, 18, 255);
                SDL_RenderFillRect(game.renderer, &life_bar);

                SDL_RenderCopy(game.renderer, button_fight.current, NULL, &button_fight.collision);
                SDL_RenderCopy(game.renderer, button_act.current, NULL, &button_act.collision);
                SDL_RenderCopy(game.renderer, button_item.current, NULL, &button_item.collision);
                SDL_RenderCopy(game.renderer, button_leave.current, NULL, &button_leave.collision);
                SDL_RenderCopy(game.renderer, battle_name.current, NULL, &battle_name.collision);
                SDL_RenderCopy(game.renderer, battle_hp.current, NULL, &battle_hp.collision);
                SDL_RenderCopy(game.renderer, battle_hp_amount.current, NULL, &battle_hp_amount.collision);

                // MR. PYTHON
                mr_python_head.collision.y = (int)(25 + 2 * sin(senoidal_timer * 1.5));
                mr_python_torso.collision.y = (int)(25 + 3 * sin(senoidal_timer * 1.5));
                mr_python_arms.collision.y = (int)(25 + 4 * sin(senoidal_timer * 1.5));

                SDL_RenderCopy(game.renderer, mr_python_arms.current, NULL, &mr_python_arms.collision);
                SDL_RenderCopy(game.renderer, mr_python_legs.current, NULL, &mr_python_legs.collision);
                SDL_RenderCopy(game.renderer, mr_python_torso.current, NULL, &mr_python_torso.collision);
                SDL_RenderCopy(game.renderer, mr_python_head.texture, NULL, &mr_python_head.collision);

                if (battle_state == ON_MENU) {
                    if (!first_dialogue) {
                        create_dialogue(&meneghetti, game.renderer, &fight_start_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                    }
                    else {
                        create_dialogue(&meneghetti, game.renderer, &fight_generic_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                        if (player_state == IDLE) player_state = IN_BATTLE;
                    }

                    if (selected_button > LEAVE) selected_button = FIGHT;
                    if (selected_button < FIGHT) selected_button = LEAVE;

                    if (keys[SDL_SCANCODE_D] && counter >= 0.2) {
                        Mix_PlayChannel(-1, move_button, 0);
                        selected_button++;
                        counter = 0.0;
                    }
                    else if (keys[SDL_SCANCODE_A] && counter >= 0.2) {
                        Mix_PlayChannel(-1, move_button, 0);
                        selected_button--;
                        counter = 0.0;
                    }

                    switch(selected_button) {
                        case FIGHT:
                            button_fight.current = fight_b_textures[1];
                            button_act.current = act_b_textures[0];
                            button_item.current = item_b_textures[0];
                            button_leave.current = leave_b_textures[0];
                            break;
                        case ACT:
                            button_fight.current = fight_b_textures[0];
                            button_act.current = act_b_textures[1];
                            button_item.current = item_b_textures[0];
                            button_leave.current = leave_b_textures[0];
                            break;
                        case ITEM:
                            button_fight.current = fight_b_textures[0];
                            button_act.current = act_b_textures[0];
                            button_item.current = item_b_textures[1];
                            button_leave.current = leave_b_textures[0];
                            break;
                        case LEAVE:
                            button_fight.current = fight_b_textures[0];
                            button_act.current = act_b_textures[0];
                            button_item.current = item_b_textures[0];
                            button_leave.current = leave_b_textures[1];
                            break;
                        default:
                            break;
                    }

                    if (e_just_pressed && counter >= 0.2) {
                        switch(selected_button) {
                            case FIGHT:
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_FIGHT;
                                break;
                            case ACT:
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_ACT;
                                break;
                            case ITEM:
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_ITEM;
                                break;
                            case LEAVE:
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_LEAVE;
                                break;
                            default:
                                break;  
                        }
                        if (!first_dialogue) {
                            first_dialogue = true;
                            player_state = IN_BATTLE;
                        }
                        counter = 0.0;
                    }
                }

                if (battle_state == ON_FIGHT) {
                    SDL_Rect perfect_hit_rect = {bar_target.collision.x + 267, bar_target.collision.y, 56, bar_target.collision.h};
                    SDL_Rect good_hit_rect = {bar_target.collision.x + 183, bar_target.collision.y, 224, bar_target.collision.h};
                    SDL_Rect normal_hit_rect = {bar_target.collision.x + 62, bar_target.collision.y, 466, bar_target.collision.h};
                    SDL_Rect bad_hit_rect = {bar_target.collision.x, bar_target.collision.y, bar_target.collision.w, bar_target.collision.h};

                    if (turn == CHOICE_TURN) {
                        soul.collision.x = text_attack_act.collision.x - soul.collision.w - 11;
                        soul.collision.y = text_attack_act.collision.y + 2;

                        SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                        SDL_RenderCopy(game.renderer, text_attack_act.current, NULL, &text_attack_act.collision);

                        if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                            Mix_PlayChannel(-1, click_button, 0);
                            battle_state = ON_MENU;
                            counter = 0.0;
                        }
                        if (e_just_pressed && counter >= 0.2) {
                            Mix_PlayChannel(-1, click_button, 0);
                            turn = ATTACK_TURN;
                            counter = 0.0;
                        }
                    }

                    if (turn == ATTACK_TURN) {
                        int attack_damage;

                        static int bar_speed = 14;
                        SDL_RenderCopy(game.renderer, bar_target.current, NULL, &bar_target.collision);
                        SDL_RenderCopy(game.renderer, bar_attack.current, NULL, &bar_attack.collision);
                        if (bar_attack.collision.x + bar_attack.collision.w > bar_target.collision.x + bar_target.collision.w - bar_speed) {
                            bar_speed = -bar_speed;
                        }
                        else if (bar_attack.collision.x < bar_target.collision.x - bar_speed) {
                            bar_speed = -bar_speed;
                        }

                        if (!tried_to_attack) {
                            if (e_just_pressed && counter >= 0.2) {
                                int w, h;
                                tried_to_attack = true;

                                damage.collision.x = py_life.x + py_life.w;
                                damage.collision.y = py_life.y - 20;
                                
                                if (rects_intersect(&bar_attack.collision, &perfect_hit_rect)) {
                                    attack_damage = meneghetti.strength * 2;
                                    damage.current = damage_numbers[3];
                                    SDL_QueryTexture(damage.current, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &good_hit_rect)) {
                                    attack_damage = meneghetti.strength * 1.5;
                                    damage.current = damage_numbers[2];
                                    SDL_QueryTexture(damage.current, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &normal_hit_rect)) {
                                    attack_damage = meneghetti.strength;
                                    damage.current = damage_numbers[1];
                                    SDL_QueryTexture(damage.current, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &bad_hit_rect)) {
                                    attack_damage = meneghetti.strength * 0.5;
                                    damage.current = damage_numbers[0];
                                    SDL_QueryTexture(damage.current, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                            }

                            bar_attack.collision.x += bar_speed;
                        }
                        else {
                            blink_timer += dt;


                            if (blink_timer <= 3.0) {
                                bar_attack.current = animate_sprite(&bar_attack_animation, dt, 0.1, &bar_attack.anim_state, false);

                                if (!Mix_Playing(0) && !has_played_slash) {
                                    Mix_PlayChannel(0, slash_sound, 0);
                                    has_played_slash = true;
                                }
                                SDL_RenderCopy(game.renderer, slash.current, NULL, &slash.collision);
                                if (slash.anim_state.counter < 5) {
                                    slash.current = animate_sprite(&slash_animation, dt, 0.2, &slash.anim_state, false);
                                    if (slash.anim_state.counter > 3) {
                                        if (!Mix_Playing(3) && !has_played_hit) {
                                            Mix_PlayChannel(3, enemy_hit_sound, 0);
                                            has_played_hit = true;
                                            mr_python_head.health -= attack_damage;
                                        }
                                        SDL_RenderCopy(game.renderer, damage.current, NULL, &damage.collision);
                                        damage.collision.y--;

                                        mr_python_head.texture = python_head_animation.frames[1];
                                        mr_python_arms.current = python_arms_animation.frames[1];
                                        mr_python_legs.current = python_legs_animation.frames[1];
                                        
                                        mr_python_head.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_torso.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_legs.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_arms.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                    }
                                }
                                else {
                                    slash.current = NULL;
                                }
                                SDL_SetRenderDrawColor(game.renderer, 168, 24, 13, 255);
                                SDL_RenderFillRect(game.renderer, &py_life_background);
                                
                                static double py_display_width = 200.0;
                                double target_width = (double)mr_python_head.health;
                                double animate_speed = 120.0;

                                if (py_display_width > target_width) {
                                    py_display_width -= animate_speed * dt;
                                    if (py_display_width < target_width) py_display_width = target_width;
                                }
                                else if (py_display_width < target_width) {
                                    py_display_width += animate_speed * dt * 2;
                                    if (py_display_width > target_width) py_display_width = target_width;
                                }

                                py_life.w = (int)(py_display_width + 0.5);

                                SDL_SetRenderDrawColor(game.renderer, 8, 207, 21, 255);
                                SDL_RenderFillRect(game.renderer, &py_life);

                                if (slash.anim_state.counter > 3) {
                                    SDL_RenderCopy(game.renderer, damage.current, NULL, &damage.collision);
                                }
                            }   
                            else {
                                mr_python_head.texture = python_head_animation.frames[0];
                                mr_python_arms.current = python_arms_animation.frames[0];
                                mr_python_legs.current = python_legs_animation.frames[0];

                                has_played_slash = false;
                                has_played_hit = false;

                                blink_timer = 0.0;
                                tried_to_attack = false;
                                turn = SOUL_TURN;
                                slash.anim_state.counter = 0;
                            }
                        }
                    }
                    if (turn == SOUL_TURN) {
                        bar_attack.collision.x = bar_target.collision.x + 20;
                        double target_w = (double)base_box.h;
                        int enemy_attack;
                        int random_dialogue;

                        if (!enemy_attack_selected) {
                            enemy_attack = randint(1, 3);
                            random_dialogue = randint(1, 3);

                            enemy_attack_selected = true;
                        }

                        if (soul_ivulnerable) {
                            ivulnerability_counter += dt;

                            soul.current = animate_sprite(&soul_animation, dt, 0.1, &soul.anim_state, false);
                            if (ivulnerability_counter >= 1.0) {
                                soul.current = soul_animation.frames[0];
                                soul_ivulnerable = false;
                                ivulnerability_counter = 0.0;
                            }
                        }

                        if (!should_expand_back && animated_box.w > target_w) {
                            animated_shrink_timer += dt;
                            if (animated_shrink_timer > animated_shrink_duration) animated_shrink_timer = animated_shrink_duration;

                            double t = animated_shrink_timer / animated_shrink_duration;
                            t = 1.0 - pow(1.0 - t, 3.0);

                            double start_w = (double)base_box.w;
                            double new_w = start_w + (target_w - start_w) * t;

                            int center_x = base_box.x + base_box.w / 2;
                            animated_box.w = (int)(new_w + 0.5);
                            animated_box.x = center_x - animated_box.w / 2;
                            animated_box.y = base_box.y;
                            animated_box.h = base_box.h;

                            soul.collision.x = (animated_box.x + (animated_box.w / 2)) - (soul.collision.w / 2);
                            soul.collision.y = (animated_box.y + (animated_box.h / 2)) - (soul.collision.h / 2);
                        }
                        else if (!should_expand_back) {
                            turn_timer += dt;
                            SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);

                            if (turn_timer <= 10.0) {
                                if (keys[SDL_SCANCODE_W]) {
                                    SDL_Rect test = soul.collision;
                                    test.y -= 2;
                                    if (!check_collision(&test, box_borders, 4)) {
                                        soul.collision.y -= 2;
                                    }
                                }
                                if (keys[SDL_SCANCODE_S]) {
                                    SDL_Rect test = soul.collision;
                                    test.y += 2;
                                    if (!check_collision(&test, box_borders, 4)) {
                                        soul.collision.y += 2;
                                    }   
                                }
                                if (keys[SDL_SCANCODE_A]) {
                                    SDL_Rect test = soul.collision;
                                    test.x -= 2;
                                    if (!check_collision(&test, box_borders, 4)) {
                                        soul.collision.x -= 2;
                                    }
                                }
                                if (keys[SDL_SCANCODE_D]) {
                                    SDL_Rect test = soul.collision;
                                    test.x += 2;
                                    if (!check_collision(&test, box_borders, 4)) {
                                        soul.collision.x += 2;
                                    }
                                }

                                if (turn_timer >= 0.5) {
                                    python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, enemy_attack, &soul_ivulnerable, python_props, dt, turn_timer, false);
                                    switch (random_dialogue) {
                                        case 1: 
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_1, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, true);
                                            break;
                                        case 2:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_2, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, true);
                                            break;
                                        case 3:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_3, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, true);
                                            break;
                                        default:
                                            break;
                                    }   
                                }
                            }
                            else {
                                python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, enemy_attack, &soul_ivulnerable, python_props, dt, turn_timer, true);
                                should_expand_back = true;
                                animated_shrink_timer = 0.0;
                                turn_timer = 0.0;
                            }
                        }
                        else if (should_expand_back) {
                            animated_shrink_timer += dt;
                            if (animated_shrink_timer > animated_shrink_duration) animated_shrink_timer = animated_shrink_duration;

                            double t = animated_shrink_timer / animated_shrink_duration;
                            t = 1.0 - pow(1.0 - t, 3.0);

                            double start_w = (double)base_box.h;
                            double target_expand_w = (double)base_box.w;
                            double new_w = start_w + (target_expand_w - start_w) * t;

                            int center_x = base_box.x + base_box.w / 2;
                            animated_box.w = (int)(new_w + 0.5);
                            animated_box.x = center_x - animated_box.w / 2;
                            animated_box.y = base_box.y;
                            animated_box.h = base_box.h;

                            if (animated_shrink_timer >= animated_shrink_duration) {
                                counter = 0.0;
                                animated_shrink_timer = 0.0;
                                should_expand_back = false;
                                turn = CHOICE_TURN;
                                battle_state = ON_MENU;
                                enemy_attack_selected = false;

                                if (current_py_damage != base_py_damage) current_py_damage = base_py_damage;

                                animated_box = base_box;
                            }
                        }
                    }
                }

                if (battle_state == ON_ACT) {
                    if (player_state == IDLE && turn != ACT_TURN) {
                        battle_state = ON_MENU;
                        turn = CHOICE_TURN;
                        player_state = IN_BATTLE;
                        on_dialogue = false;
                        counter = 0.0;
                        menu_pos = 0;
                    }
                    else {
                        if (turn == CHOICE_TURN) {
                            soul.collision.x = text_attack_act.collision.x - soul.collision.w - 11;
                            soul.collision.y = text_attack_act.collision.y + 2;

                            SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_attack_act.current, NULL, &text_attack_act.collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                turn = ACT_TURN;
                                counter = 0.0;
                            }
                        }
                        if (turn == ACT_TURN) {
                            if (!on_dialogue) {
                                
                                if (menu_pos > 2) menu_pos = 0;
                                if (menu_pos < 0) menu_pos = 2;

                                switch(menu_pos) {
                                case 0:
                                    soul.collision.x = text_act[0].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_act[0].collision.y + 2;
                                    break;
                                case 1:
                                    soul.collision.x = text_act[1].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_act[1].collision.y + 2;
                                    break;
                                case 2:
                                    soul.collision.x = text_act[2].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_act[2].collision.y + 2;
                                    break;
                                default:
                                    break;
                                }

                                SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                                SDL_RenderCopy(game.renderer, text_act[0].current, NULL, &text_act[0].collision);
                                SDL_RenderCopy(game.renderer, text_act[1].current, NULL, &text_act[1].collision);
                                SDL_RenderCopy(game.renderer, text_act[2].current, NULL, &text_act[2].collision);

                                if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                    Mix_PlayChannel(-1, click_button, 0);
                                    turn = CHOICE_TURN;
                                    counter = 0.0;
                                }
                                if (e_just_pressed && counter >= 0.2) {
                                    Mix_PlayChannel(-1, click_button, 0);
                                    switch(menu_pos) {
                                        case 1:
                                            if (first_insult) {
                                                current_py_damage += 2;
                                            }
                                            break;
                                        case 2:
                                            if (first_explain) {
                                                current_py_damage -= 1;
                                            }
                                            break;
                                        default:
                                            break;
                                    }
                                    on_dialogue = true;
                                    counter = 0.0;
                                }
                                if (keys[SDL_SCANCODE_S] && counter >= 0.2) {
                                    Mix_PlayChannel(-1, move_button, 0);
                                    menu_pos++;
                                    counter = 0.0;
                                }
                                if (keys[SDL_SCANCODE_W] && counter >= 0.2) {
                                    Mix_PlayChannel(-1, move_button, 0);
                                    menu_pos--;
                                    counter = 0.0;
                                }
                                if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_A]) {
                                if (counter >= 0.2) {
                                    Mix_PlayChannel(-1, move_button, 0);
                                    switch(menu_pos) {
                                        case 0:
                                            menu_pos = 2;
                                            break;
                                        case 2:
                                            menu_pos = 0;
                                            break;
                                        default:
                                            break;
                                    }
                                    counter = 0.0;
                                }
                            }
                            }
                            else {
                                switch(menu_pos) {
                                case 0:
                                    create_dialogue(&meneghetti, game.renderer, &fight_act_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    break;
                                case 1:
                                    if (first_insult) {
                                        create_dialogue(&meneghetti, game.renderer, &insult_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    }
                                    else {
                                        create_dialogue(&meneghetti, game.renderer, &insult_generic_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    }
                                    break;
                                case 2:
                                    if (first_explain) {
                                        create_dialogue(&meneghetti, game.renderer, &explain_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    }
                                    else {
                                        create_dialogue(&meneghetti, game.renderer, &explain_generic_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    }
                                    break;
                                default:
                                    break;
                                }

                                if (player_state == IDLE) {
                                    on_dialogue = false;
                                    if (menu_pos == 1 && first_insult) {
                                        turn = SOUL_TURN;
                                        battle_state = ON_FIGHT;
                                        player_state = IN_BATTLE;
                                    }
                                    else if (menu_pos == 2 && first_explain) {
                                        turn = SOUL_TURN;
                                        battle_state = ON_FIGHT;
                                        player_state = IN_BATTLE;
                                    }
                                    else {
                                        turn = ACT_TURN;
                                        player_state = IN_BATTLE;
                                    }

                                    if (first_insult && menu_pos == 1) first_insult = false;
                                    if (first_explain && menu_pos == 2) first_explain = false;
                                    counter = 0.0;
                                    menu_pos = 0;
                                }
                            }
                        }
                    }
                }

                if (battle_state == ON_ITEM) {
                    if (player_state == IDLE) {
                        battle_state = ON_MENU;
                        player_state = IN_BATTLE;
                        on_dialogue = false;
                        has_played_eat = false;
                        counter = 0.0;
                        menu_pos = 0;
                        if (food_amount > 0) food_amount--;
                    }
                    else {
                        if (!on_dialogue && food_amount > 0) {
                            soul.collision.x = text_item.collision.x - soul.collision.w - 11;
                            soul.collision.y = text_item.collision.y + 2;
                            food_amount_text.collision.x = text_item.collision.x + text_item.collision.w + 5;
                            food_amount_text.collision.y = text_item.collision.y;

                            SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_item.current, NULL, &text_item.collision);
                            SDL_RenderCopy(game.renderer, food_amount_text.current, NULL, &food_amount_text.collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                meneghetti.health += 20;
                                on_dialogue = true;
                                counter = 0.0;
                            }
                        }
                        else {
                            if (!Mix_Playing(2) && !has_played_eat && food_amount > 0) {
                                Mix_PlayChannel(2, eat_sound, 0);
                                has_played_eat = true;
                            }
                            if (food_amount > 0) {
                                create_dialogue(&meneghetti, game.renderer, &picanha_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                            }
                            else {
                                create_dialogue(&meneghetti, game.renderer, &no_food_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                            }
                        }   
                    }
                }

                if (battle_state == ON_LEAVE) {
                    if (player_state == IDLE) {
                        battle_state = ON_MENU;
                        player_state = IN_BATTLE;
                        on_dialogue = false;
                        counter = 0.0;
                        menu_pos = 0;
                    }
                    else {
                        if (!on_dialogue) {
                            if (menu_pos > 1) menu_pos = 0;
                            if (menu_pos < 0) menu_pos = 1;

                            switch(menu_pos) {
                                case 0:
                                    soul.collision.x = text_leave[0].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_leave[0].collision.y + 2;
                                    break;
                                case 1:
                                    soul.collision.x = text_leave[1].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_leave[1].collision.y + 2;
                                    break;
                                default:
                                    break;
                            }

                            SDL_RenderCopy(game.renderer, soul.current, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_leave[0].current, NULL, &text_leave[0].collision);
                            SDL_RenderCopy(game.renderer, text_leave[1].current, NULL, &text_leave[1].collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(-1, click_button, 0);
                                on_dialogue = true;
                                counter = 0.0;
                            }
                            if (keys[SDL_SCANCODE_S] && counter >= 0.2) {
                                Mix_PlayChannel(-1, move_button, 0);
                                menu_pos++;
                                counter = 0.0;
                            }
                            if (keys[SDL_SCANCODE_W] && counter >= 0.2) {
                                Mix_PlayChannel(-1, move_button, 0);
                                menu_pos--;
                                counter = 0.0;
                            }
                        }
                        else {
                            switch(menu_pos) {
                                case 0:
                                    create_dialogue(&meneghetti, game.renderer, &fight_spare_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    break;
                                case 1:
                                    create_dialogue(&meneghetti, game.renderer, &fight_leave_txt, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                if (mr_python_head.health <= 0) {
                    mr_python_head.health = 0;
                    battle_state = ON_MENU;
                    turn = CHOICE_TURN;

                    mr_python_head.texture = python_head_animation.frames[1];
                    mr_python_arms.current = python_arms_animation.frames[1];
                    mr_python_legs.current = python_legs_animation.frames[1];
                    
                    mr_python_head.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                    mr_python_torso.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                    mr_python_legs.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                    mr_python_arms.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);

                    if (end_scene_fade.fading_in) {
                        end_scene_fade.timer += dt;
                        end_scene_fade.alpha = 0 + (Uint8)((end_scene_fade.timer / 5.0) * 255);

                        if (end_scene_fade.timer >= 5.0) {
                            end_scene_fade.alpha = 255;
                            end_scene_fade.fading_in = false;
                        }
                    }
                    else {
                        end_scene_fade.timer = 0.0;
                        python_dead = true;
                    }

                    if (end_scene_fade.alpha < 255) {
                        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, end_scene_fade.alpha);
                        SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);

                        SDL_Rect screen_fade = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
                        SDL_RenderFillRect(game.renderer, &screen_fade);
                    }
                }
            }
            if (player_state == DEAD || python_dead) {
                Mix_HaltChannel(5);

                senoidal_timer = 0.0;
                counter = 0.0;
                battle_ready = false;
                on_dialogue = false;
                first_dialogue = false;
                animated_box_inited = false;
                animated_shrink_timer = 0.0;
                selected_button = FIGHT;
                turn = CHOICE_TURN;
                battle_state = ON_MENU;
                menu_pos = 0;
                current_py_damage = base_py_damage;
                food_amount = 4;
                enemy_attack_selected = false;
                tried_to_attack = false;
                blink_timer = 0.0;
                has_played_slash = false;
                has_played_hit = false;
                soul_ivulnerable = false;
                ivulnerability_counter = 0.0;
                turn_timer = 0.0;
                should_expand_back = false;
                first_insult = true;
                first_explain = true;
                has_played_eat = false;

                python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, 0, &soul_ivulnerable, python_props, dt, turn_timer, true);

                int attack_widths, attack_heights;
                command_rain[0].current = create_texture(game.renderer, "assets/sprites/battle/if.png");
                SDL_QueryTexture(command_rain[0].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                command_rain[1].current = create_texture(game.renderer, "assets/sprites/battle/else.png");
                SDL_QueryTexture(command_rain[1].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                command_rain[2].current = create_texture(game.renderer, "assets/sprites/battle/elif.png");
                SDL_QueryTexture(command_rain[2].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                command_rain[3].current = create_texture(game.renderer, "assets/sprites/battle/input.png");
                SDL_QueryTexture(command_rain[3].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                command_rain[4].current = create_texture(game.renderer, "assets/sprites/battle/print.png");
                SDL_QueryTexture(command_rain[4].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[4].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                command_rain[5].current = create_texture(game.renderer, "assets/sprites/battle/in.png");
                SDL_QueryTexture(command_rain[5].current, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[5].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

                parenthesis_enclosure[0].current = create_texture(game.renderer, "assets/sprites/battle/brackets-1.png");
                SDL_QueryTexture(parenthesis_enclosure[0].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                parenthesis_enclosure[1].current = create_texture(game.renderer, "assets/sprites/battle/brackets-2.png");
                SDL_QueryTexture(parenthesis_enclosure[1].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                parenthesis_enclosure[2].current = create_texture(game.renderer, "assets/sprites/battle/key-1.png");
                SDL_QueryTexture(parenthesis_enclosure[2].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                parenthesis_enclosure[3].current = create_texture(game.renderer, "assets/sprites/battle/key-2.png");
                SDL_QueryTexture(parenthesis_enclosure[3].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                parenthesis_enclosure[4].current = create_texture(game.renderer, "assets/sprites/battle/parenthesis-1.png");
                SDL_QueryTexture(parenthesis_enclosure[4].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[4].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                parenthesis_enclosure[5].current = create_texture(game.renderer, "assets/sprites/battle/parenthesis-2.png");
                SDL_QueryTexture(parenthesis_enclosure[5].current, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[5].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

                python_mother[0].current = python_mother_animation.frames[0];
                SDL_QueryTexture(python_mother[0].current, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[0].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                python_mother[1].current = python_mother_animation.frames[1];
                SDL_QueryTexture(python_mother[1].current, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[1].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                python_mother[2].current = python_baby_animation.frames[0];
                SDL_QueryTexture(python_mother[2].current, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[2].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};
                python_mother[3].current = python_baby_animation.frames[1];
                SDL_QueryTexture(python_mother[3].current, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[3].collision = (SDL_Rect){0, 0, attack_widths, attack_heights};

                python_props[0] = command_rain;
                python_props[1] = parenthesis_enclosure;
                python_props[2] = python_mother;

                soul.anim_state.counter = 0;
                slash.anim_state.counter = 0;
                mr_python_head.health = 200;
                bar_attack.collision.x = bar_target.collision.x + 20;

                if (player_state == DEAD) {
                    game_state = DEATH_SCREEN;
                }
                else if (python_dead) {
                    game_state = FINAL_SCREEN;
                    end_scene_fade.timer = 0.0;
                    end_scene_fade.alpha = 255;
                    end_scene_fade.fading_in = true;
                    python_dead = false;
                }
            }
        }

        if (game_state == DEATH_SCREEN) {
            death_counter += dt;
            SDL_RenderClear(game.renderer);
            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);

            if (death_counter <= 2.0) {
                if (!has_played_break) {
                    Mix_PlayChannel(-1, soul_break_sound, 0);
                    has_played_break = true;
                }
                SDL_RenderCopy(game.renderer, soul_shattered.current, NULL, &soul.collision);
            }
            else {
                open_world_fade.alpha = (Uint8)255;
                open_world_fade.fading_in = true;
                open_world_fade.timer = 0.0;
                death_counter = 0.0;

                has_played_break = false;
                meneghetti_arrived = true;
                player_state = MOVABLE;
                game_state = OPEN_WORLD;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                
                delay_started = false;
                car_animation_timer = 0.0;
                arrival_timer = 0.0;
                python_dialogue_finished = false;
                first_dialogue = false;

                meneghetti.health = 20;
                last_health = meneghetti.health;

                SDL_RenderClear(game.renderer);
            }
        }

        if (game_state == FINAL_SCREEN) {
            if (end_scene_fade.alpha > 0) {
                end_scene_fade.timer += dt;
                end_scene_fade.alpha = 255 - (Uint8)((end_scene_fade.timer / 3.0) * 255);
                if (end_scene_fade.timer >= 3.0) {
                    end_scene_fade.alpha = 0;
                }
            }

            SDL_RenderClear(game.renderer);
            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);

            create_dialogue(&meneghetti, game.renderer, &end_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);

            if (end_scene_fade.alpha > 0) {
                SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, end_scene_fade.alpha);
                SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);

                SDL_Rect screen_fade = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
                SDL_RenderFillRect(game.renderer, &screen_fade);
            }
            if (player_state == MOVABLE) {
                open_world_fade.alpha = (Uint8)255;
                open_world_fade.fading_in = true;
                open_world_fade.timer = 0.0;
                death_counter = 0.0;

                meneghetti_arrived = false;
                player_state = IDLE;
                game_state = TITLE_SCREEN;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                
                delay_started = false;
                car_animation_timer = 0.0;
                arrival_timer = 0.0;
                python_dialogue_finished = false;
                first_dialogue = false;

                meneghetti.health = 20;
                last_health = meneghetti.health;

                SDL_RenderClear(game.renderer);
            }
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

SDL_Texture *create_txt(SDL_Renderer *render, const char *utf8_text, TTF_Font *font, SDL_Color color) {
    if (!utf8_text || !utf8_text[0]) return NULL;

    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, utf8_text, color);
    if (!surface) {
        fprintf(stderr, "Error loading text surface (text '%s'): %s", utf8_text, TTF_GetError());
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

SDL_Texture *animate_sprite(Animation *anim, double dt, double cooldown, AnimState *state, bool blink) {
    if (!anim || anim->count <= 0) return NULL;

    if (cooldown <= 0.0) {
        state->counter = (state->counter + 1) % anim->count;
        state->timer = 0.0;
        return anim->frames[state->counter];
    }

    state->timer += dt;

    int steps = (int)(state->timer / cooldown);
    if (steps > 0) {
        state->counter = (state->counter + steps) % anim->count;
        state->timer -= (double)steps * cooldown;
    }

    if (blink && anim->count == 2) {
        if (state->counter == 1) {
            double blink_duration = cooldown / 7.0;
            if (state->timer >= blink_duration) {
                state->counter = 0;
            }
        }
        else {
            state->counter = 0;
        }
    }

    state->counter %= anim->count;
    return anim->frames[state->counter];

}

void create_dialogue(Character *player, SDL_Renderer *render, Text *text, int *player_state, int *game_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Mix_Chunk **sound, bool bubble) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    
    bool has_meneghetti = (meneghetti_face != NULL);
    bool has_python = (python_face != NULL);
    
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
    
    SDL_Rect dialogue_box = {0, 0, 0, 0};
    Prop bubble_speech = {
        .current = create_texture(render, "assets/sprites/battle/text-bubble.png"),
        .collision = dialogue_box
    };
    switch(*game_state) {
        case CUTSCENE:
            dialogue_box = (SDL_Rect){20, SCREEN_HEIGHT - 200, SCREEN_WIDTH - 40, 180};
            break;
        case OPEN_WORLD:
            if (player->collision.y + player->collision.h < SCREEN_HEIGHT / 2) {
                dialogue_box = (SDL_Rect){25, SCREEN_HEIGHT - 175, SCREEN_WIDTH - 50, 150};
            }
            else {
                dialogue_box = (SDL_Rect){25, 25, SCREEN_WIDTH - 50, 150};
            }
            break;
        case BATTLE_SCREEN:
            if (bubble) {
                int w, h;
                SDL_QueryTexture(bubble_speech.current, NULL, NULL, &w, &h);
                dialogue_box = (SDL_Rect){380, 40, w * 1.5, h * 1.5};
            }
            else {
                dialogue_box = (SDL_Rect){20, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 40, 132};
            }
            break;
        case FINAL_SCREEN:
            dialogue_box = (SDL_Rect){25, SCREEN_HEIGHT - 175, SCREEN_WIDTH - 50, 150};
            break;
        default:
            break;
    }

    if (*game_state != BATTLE_SCREEN) {
        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderFillRect(render, &dialogue_box);
    }
    if (bubble) {
        SDL_RenderCopy(render, bubble_speech.current, NULL, &dialogue_box);
    }

    // BORDAS:
    if (*game_state != CUTSCENE && *game_state != BATTLE_SCREEN && *game_state != FINAL_SCREEN) {
        SDL_Rect box_borders[] = {{dialogue_box.x, dialogue_box.y, dialogue_box.w, 5}, {dialogue_box.x, dialogue_box.y, 5, dialogue_box.h}, {dialogue_box.x, dialogue_box.y + dialogue_box.h - 5, dialogue_box.w, 5}, {dialogue_box.x + dialogue_box.w - 5, dialogue_box.y, 5, dialogue_box.h}};
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderFillRects(render, box_borders, 4);
    }

    SDL_Rect meneghetti_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 72, 96};
    SDL_Rect python_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 96, 96};

    if (text->on_frame[text->cur_str] != NONE) {
        text->text_box.x = dialogue_box.x + 130;
        text->text_box.y = dialogue_box.y + 27;
    }
    else {
        if (bubble) {
            text->text_box.x = dialogue_box.x + 35;
            text->text_box.y = dialogue_box.y + 5;
        }
        else {
            text->text_box.x = dialogue_box.x + 27;
            text->text_box.y = dialogue_box.y + 27;
        }
    }

    static double sfx_timer = 0.0;
    const double sfx_cooldown = 0.03;
    sfx_timer += dt;

    if (!text->waiting_for_input) {
        *anim_timer += dt;
        text->timer += dt;
        if (e_pressed && *game_state != BATTLE_SCREEN && !bubble) {
            const char *current = text->writings[text->cur_str];
            while (current[text->cur_byte] != '\0' && text->char_count < MAX_DIALOGUE_CHAR) {
                char utf8_buffer[5];
                int n = utf8_copy_char(&current[text->cur_byte], utf8_buffer);
                SDL_Texture* t = create_txt(render, utf8_buffer, text->text_font, text->text_color);
                if (t) {
                    text->chars[text->char_count] = t;
                    strcpy(text->chars_string[text->char_count], utf8_buffer);
                    text->char_count++;
                }
                text->cur_byte += n;
            }
            text->waiting_for_input = true;
        }
        else if (text->timer >= timer_delay) {
            text->timer = 0.0;
            const char *current = text->writings[text->cur_str];
            
            if (current[text->cur_byte] == '\0') {
                text->waiting_for_input = true;
            }
            else {
                if (text->char_count < MAX_DIALOGUE_CHAR) {
                    char utf8_buffer[5];
                    int n = utf8_copy_char(&current[text->cur_byte], utf8_buffer);
                    SDL_Texture* t = create_txt(render, utf8_buffer, text->text_font, text->text_color);
                    if (t) {
                        if (sound && sfx_timer >= sfx_cooldown) {
                            int speaker = text->on_frame[text->cur_str];
                            Mix_Chunk* chunk =  NULL;
                            
                            if (speaker == MENEGHETTI || speaker == MENEGHETTI_ANGRY || speaker ==  MENEGHETTI_SAD) {
                                chunk = sound[0];
                            }
                            if (speaker == PYTHON) {
                                chunk = sound[1];
                            }
                            if (speaker == NONE) {
                                chunk = sound[2];
                            }

                            if (chunk) {
                                Mix_PlayChannel(1, chunk, 0);
                            }
                            sfx_timer = 0.0;
                        }
                        text->chars[text->char_count] = t;
                        strcpy(text->chars_string[text->char_count], utf8_buffer);
                        text->char_count++;
                    }
                    text->cur_byte += n;
                }
            }
        }
    }
    else {
        if (e_pressed && !bubble) {
            for (int i = 0; i < text->char_count; i++) {
                if (text->chars[i]) {
                    SDL_DestroyTexture(text->chars[i]);
                    text->chars[i] = NULL;
                }
            }
            text->char_count = 0;
            text->cur_byte = 0;
            text->cur_str++;

            text->waiting_for_input = false;
            if (text->cur_str >= text_amount) {
                reset_dialogue(text);
                
                if (*game_state == BATTLE_SCREEN) *player_state = IDLE;
                else *player_state = MOVABLE;

                return;
            }
        }
    }

    int pos_x = text->text_box.x;
    int pos_y = text->text_box.y;

    int max_x;
    if (bubble) max_x = dialogue_box.x + dialogue_box.w - 2;
    else max_x = dialogue_box.x + dialogue_box.w - 50;

    int max_y;
    if (*game_state != BATTLE_SCREEN) max_y = dialogue_box.y + dialogue_box.h - 27;
    else if (bubble) max_y = dialogue_box.y + dialogue_box.h - 2;
    else max_y = dialogue_box.y + dialogue_box.h;

    int default_line_height = 16;
    if (text->text_font) {
        default_line_height = TTF_FontHeight(text->text_font);
    } 
    int line_height = 0;

    for (int i = 0; i < text->char_count; i++) {
        SDL_Texture* ct = text->chars[i];
        if (!ct) continue;

        int w, h;
        SDL_QueryTexture(ct, NULL, NULL, &w, &h);
        if (h > line_height) line_height = h;
    }

    int current_x = pos_x;
    int current_y = pos_y;
    int word_width = 0;
    int word_start = -1;

    for (int i = 0; i < text->char_count; i++) {
        SDL_Texture* ct = text->chars[i];
        if (!ct) continue;

        int w, h;
        SDL_QueryTexture(ct, NULL, NULL, &w, &h);
        if (w == 0) w = 8;
        if (h == 0) h = 16;

        const char *chstr = text->chars_string[i];
        bool is_space = (strcmp(chstr, " ") == 0);
        bool is_newline = (strcmp(chstr, "|") == 0);

        if (is_newline) {
            word_start = -1;
            word_width = 0;

            current_x = pos_x;
            current_y += (line_height > 0) ? line_height : default_line_height;
            line_height = 0;
            continue;
        }

        if (!is_space && word_start == -1) {
            word_start = i;
            word_width = 0;
        }

        if (!is_space) {
            word_width += w;
        }

        if (is_space || i == text->char_count - 1) {
            if (word_start != -1) {
                if (current_x + word_width > max_x) {
                    current_x = pos_x;
                    current_y += (line_height > 0) ? line_height : default_line_height;
                    line_height = 0;

                    for (int j = word_start; j <= i; j++) {
                        SDL_Texture* char_tex = text->chars[j];
                        if (!char_tex) continue;

                        int ch;
                        SDL_QueryTexture(char_tex, NULL, NULL, NULL, &ch);
                        if (ch > line_height) line_height = ch;
                    }
                }

                for (int j = word_start; j <= i; j++) {
                    SDL_Texture* char_tex = text->chars[j];
                    if (!char_tex) continue;

                    int cw, ch;
                    SDL_QueryTexture(char_tex, NULL, NULL, &cw, &ch);
                    SDL_Rect dst = {current_x, current_y, cw, ch};
                    SDL_RenderCopy(render, char_tex, NULL, &dst);
                    current_x += cw;
                }

                word_start = -1;
                word_width = 0;
            }

            if (is_space) {
                if (current_x + w > max_x) {
                    current_x = pos_x;
                    current_y += (line_height > 0) ? line_height : default_line_height;
                    line_height = h;
                }

                SDL_Rect dst = {current_x, current_y, w, h};
                SDL_RenderCopy(render, ct, NULL, &dst);
                current_x += w;
            }
        }

        if (current_y + line_height > max_y) {
            text->waiting_for_input = true;
            break;
        }
    }

    static int counters[] = {0, 0};
    if (text->on_frame[text->cur_str] != NONE) {
        switch(text->on_frame[text->cur_str]) {
            case MENEGHETTI:
                if (has_meneghetti && meneghetti_face[0].count > 0 && meneghetti_face[0].frames) {
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
                }
                break;
            case MENEGHETTI_ANGRY:
                if (has_meneghetti && meneghetti_face[1].count > 0 && meneghetti_face[1].frames) {
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
                }
                break;
            case MENEGHETTI_SAD:
                if (has_meneghetti && meneghetti_face[2].count > 0 && meneghetti_face[2].frames) {
                    if (!text->waiting_for_input) {
                        while (*anim_timer >= anim_cooldown) {
                            
                            counters[0] = (counters[0] + 1) % meneghetti_face[2].count;
                            *anim_timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, meneghetti_face[2].frames[counters[0] % meneghetti_face[2].count], NULL, &meneghetti_frame);
                    }
                    else {
                        counters[0] = 0;
                        SDL_RenderCopy(render, meneghetti_face[2].frames[0], NULL, &meneghetti_frame);
                    }
                }
                break;
            case PYTHON:
                if (has_python && python_face->count > 0 && python_face->frames) {
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
    text->cur_byte = 0;
    text->timer = 0.0;
    text->waiting_for_input = false;

}

void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], double *anim_timer, double anim_interval, Mix_Chunk **sound) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);

    SDL_Rect feet = {player->collision.x, player->collision.y + 29, player->collision.w, 3};
    
    bool raw_up = keys[SDL_SCANCODE_W];
    bool raw_down = keys[SDL_SCANCODE_S];
    bool raw_left = keys[SDL_SCANCODE_A];
    bool raw_right = keys[SDL_SCANCODE_D];

    bool up = raw_up && !raw_down;
    bool down = raw_down && !raw_up;
    bool left = raw_left && !raw_right;
    bool right = raw_right && !raw_left;

    float move_vel = player->sprite_vel * (float)dt;
    int move = (int)roundf(move_vel);

    if (move == 0 && move_vel != 0.0f) move = (move_vel > 0.0f) ? 1 : -1;

    *anim_timer += dt;

    bool moving_up = false, moving_down = false, moving_left = false, moving_right = false;

    if (up) {
        player->facing = UP;
    }
    else if (down) {
        player->facing = DOWN;
    }
    else if (left) {
        player->facing = LEFT;
    }
    else if (right) {
        player->facing = RIGHT;
    }
    
    if (up) {
        if (scenario->collision.y < 0 && player->collision.y < (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = feet;
            test.y -= move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY)) {
                scenario->collision.y += move;
                moving_up = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y - 6;
        } else {
            SDL_Rect test = feet;
            test.y -= move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY) && player->collision.y > 0) {
                player->collision.y -= move;
                moving_up = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y - 6;
        }
    }

    if (down) {
        if (scenario->collision.y > -SCREEN_HEIGHT && player->collision.y > (SCREEN_HEIGHT / 2) - 16) {
            SDL_Rect test = feet;
            test.y += move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY)) {
                scenario->collision.y -= move;
                moving_down = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y + player->collision.h;
        } else {
            SDL_Rect test = feet;
            test.y += move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY) && player->collision.y < SCREEN_HEIGHT - player->collision.h) {
                player->collision.y += move;
                moving_down = true;
            }
            player->interact_collision.x = player->collision.x;
            player->interact_collision.y = player->collision.y + player->collision.h;
        }
    }

    if (left) {
        if (scenario->collision.x < 0 && player->collision.x < (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = feet;
            test.x -= move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY)) {
                scenario->collision.x += move;
                moving_left = true;
            }
            player->interact_collision.x = player->collision.x - player->interact_collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        } else {
            SDL_Rect test = feet;
            test.x -= move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY) && player->collision.x > 0) {
                player->collision.x -= move;
                moving_left = true;
            }
            player->interact_collision.x = player->collision.x - player->interact_collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        }
    }

    if (right) {
        if (scenario->collision.x > -SCREEN_WIDTH && player->collision.x > (SCREEN_WIDTH / 2) - 10) {
            SDL_Rect test = feet;
            test.x += move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY)) {
                scenario->collision.x -= move;
                moving_right = true;
            }
            player->interact_collision.x = player->collision.x + player->collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        } else {
            SDL_Rect test = feet;
            test.x += move;
            if (!check_collision(&test, boxes, COLLISION_QUANTITY) && player->collision.x < SCREEN_WIDTH - player->collision.w) {
                player->collision.x += move;
                moving_right = true;
            }
            player->interact_collision.x = player->collision.x + player->collision.w;
            player->interact_collision.y = (player->collision.y + player->collision.h) - player->interact_collision.h;
        }
    }

    if (moving_up) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[UP].frames[ player->counters[UP] % animation[UP].count ];
            player->counters[UP] = (player->counters[UP] + 1) % animation[UP].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_down) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[DOWN].frames[ player->counters[DOWN] % animation[DOWN].count ];
            player->counters[DOWN] = (player->counters[DOWN] + 1) % animation[DOWN].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_left) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[LEFT].frames[ player->counters[LEFT] % animation[LEFT].count ];
            player->counters[LEFT] = (player->counters[LEFT] + 1) % animation[LEFT].count;
            *anim_timer = 0.0;
        }
    }
    else if (moving_right) {
        if (*anim_timer >= anim_interval) {
            player->texture = animation[RIGHT].frames[ player->counters[RIGHT] % animation[RIGHT].count ];
            player->counters[RIGHT] = (player->counters[RIGHT] + 1) % animation[RIGHT].count;
            *anim_timer = 0.0;
        }
    }
    else {
        player->counters[UP] = player->counters[DOWN] = player->counters[LEFT] = player->counters[RIGHT] = 0;

        int face = player->facing;
        if (face < 0 || face >= DIR_COUNT) face = DOWN;
        player->texture = animation[face].frames[0];
    }

    static int current_walk_sound = -1;

    if (moving_up || moving_down || moving_left || moving_right) {
        int surface_index = detect_surface(&player->collision, surfaces, SURFACE_QUANTITY);
        int new_sound_index = surface_to_sound_index(surface_index);

        if (new_sound_index != current_walk_sound) {
            if (Mix_Playing(0)) {
                Mix_HaltChannel(0);
            }

            if (new_sound_index != -1) {
                Mix_PlayChannel(0, sound[new_sound_index], -1);
            }
        }
        current_walk_sound = new_sound_index;
    }
    else {
        if (Mix_Playing(0)) {
            Mix_HaltChannel(0);
        }

        current_walk_sound = -1;
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

static int detect_surface(SDL_Rect *player, SDL_Rect surfaces[], int surface_count) {
    SDL_Rect feet = {player->x, player->y + 29, player->w, 3};

    int best_index = -1;
    int best_overlap = 0;

    for (int i = 0; i < surface_count; i++) {
        SDL_Rect s = surfaces[i];

        int ix = SDL_max(feet.x, s.x);
        int iy = SDL_max(feet.y, s.y);
        int ax = SDL_min(feet.x + feet.w, s.x + s.w);
        int ay = SDL_min(feet.y + feet.h, s.y + s.h);

        int overlap_w = ax - ix;
        int overlap_h = ay - iy;
        if (overlap_w > 0 && overlap_h > 0) {
            int area = overlap_w * overlap_h;
            if (area > best_overlap) {
                best_overlap = area;
                best_index = i;
            }
        }
    }

    return best_index;

}

static int surface_to_sound_index(int surface_index) {
    if (surface_index < 0 || surface_index > 12) return -1;
    if (surface_index == 0 || surface_index == 1 || surface_index == 2) return 0;
    if (surface_index == 3  || surface_index == 4 || surface_index == 5) return 1;
    if (surface_index == 6) return 2;
    if (surface_index == 7) return 3;
    if (surface_index == 8) return 4;
    if (surface_index == 9 || surface_index == 10 || surface_index == 11 || surface_index == 12) return 5;

    return -1;

}

static int utf8_charlen(const char *s) {
    unsigned char c = (unsigned char)s[0];
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;

}

static int utf8_copy_char(const char *s, char *out) {
    int n = utf8_charlen(s);
    for (int i = 0; i < n; i++) {
        out[i] = s[i];
    }
    out[n] = '\0';
    return n;

}

int renderitem_cmp(const void *pa, const void *pb) {
    const RenderItem *a = (const RenderItem *)pa;
    const RenderItem *b = (const RenderItem *)pb;

    int da = a->collisions->y + a->collisions->h;
    int db = b->collisions->y + b->collisions->h;
    if (da < db) return -1;
    if (da > db) return 1;
    
    return 0;

}

void update_reflection(Character *original, Character* reflection, Animation *animation) {
    reflection->facing = original->facing;

    int current_frame = original->counters[original->facing];

    reflection->texture = animation[reflection->facing].frames[current_frame];

    reflection->collision.x = original->collision.x;
    reflection->collision.y = original->collision.y + original->collision.h;
    reflection->collision.w = original->collision.w;
    reflection->collision.h = original->collision.h;

}

void organize_items(Prop *text_items) {
    int available = 0;
    for (int i = 0; i < 4; i++) {
        if (text_items[i].current != NULL) available++;
    }
    if (available == 4) return;

    int x_col1 = 69;
    int x_col2 = 69 + 200;

    int base_y = -1;
    for (int i = 0; i < 4; i++) {
        if (text_items[i].current != NULL) {
            base_y = (SCREEN_HEIGHT / 2) + 25;
            break;
        }
    }
    if (base_y == -1) return;
    
    Prop *col1[2] = {NULL, NULL};
    int col1_count = 0;

    Prop *col2[2] = {NULL, NULL};
    int col2_count = 0;

    if (text_items[0].current != NULL) col1[col1_count++] = &text_items[0];
    if (text_items[1].current != NULL) col1[col1_count++] = &text_items[1];
    if (text_items[2].current != NULL) col2[col2_count++] = &text_items[2];
    if (text_items[3].current != NULL) col2[col2_count++] = &text_items[3];

    int current_y = base_y;
    for (int i = 0; i < 2; i++) {
        if (i < col1_count && col1[i] != NULL) {
            col1[i]->collision.x = x_col1;
            col1[i]->collision.y = current_y;
            current_y += col1[i]->collision.h + 10;
        }
    }

    current_y = base_y;
    for (int i = 0; i < 2; i++) {
        if (i < col2_count && col2[i] != NULL) {
            col2[i]->collision.x = x_col2;
            col2[i]->collision.y = current_y;
            current_y += col2[i]->collision.h + 10; 
        }
    }

    if (col1_count == 0 && col2_count > 0) {
        current_y = base_y;
        for (int i = 0; i < col2_count; i++) {
            if (col2[i] != NULL) {
                col2[i]->collision.x = x_col1;
                col2[i]->collision.y = current_y;
                current_y += col2[i]->collision.h + 10;
            }
        }
    }

    Prop tmp[4];
    for (int i = 0; i < 4; i++) {
        tmp[i].current = NULL;
        tmp[i].collision = (SDL_Rect){0, 0, 0, 0};
    }

    int idx = 0;
    for (int i = 0; i < col1_count; i++) {
        if (col1[i] != NULL) {
            tmp[idx++] = *col1[i];
        }
    }

    for (int i = 0; i < col2_count; i++) {
        if (col2[i] != NULL) {
            tmp[idx++] = *col2[i];
        }
    }

    for (int i = 0; i < 4; i++) {
        text_items[i] = tmp[i];
    }

}

int randint(int min, int max) {
    return min + rand() % (max - min + 1);

}

int choice(int count, ...) {
    va_list args;
    va_start(args, count);

    int index = rand() % count;
    int result = 0;

    for (int i = 0; i < count; i++) {
        int value = va_arg(args, int);
        if (i == index) {
            result = value;
        }
    }

    va_end(args);
    return result;

}

// ATAQUES DO MR. PYTHON
void python_attacks(SDL_Renderer *render, Prop *soul, SDL_Rect battle_box, int *player_health, int damage, int attack_index, bool *ivulnerable, Prop **props, double dt, double turn_timer, bool clear) {
    static double spawn_timer = 0.0;
    static int objects_spawned = 0;
    static bool attack_active = false;
    
    static Prop active_objects[15];
    static bool created_object[15] = {false};
    static double objects_speed[15] = {0};

    static double vel_x[15] = {0};
    static double vel_y[15] = {0};
    static double angles[15] = {0};

    static int alpha_counter = 0;

    Mix_Chunk* hit_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/damage_taken.wav");
    Mix_Chunk* appear_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/object_appears.wav");
    Mix_Chunk* born_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/python_ejects.wav");
    Mix_Chunk* slam_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/slam.wav");
    Mix_Chunk* strike_sound = Mix_LoadWAV("assets/sounds/sound_effects/battle-sounds/strike_sound.wav");

    static bool played_appear_sound = false;

    if (!clear) {    
        switch(attack_index) {
            case 1:
                if (!attack_active && turn_timer <= 8.0) {
                    attack_active = true;
                    spawn_timer = 0.0;
                    objects_spawned = 0;

                    for (int i = 0; i < 15; i++) {
                        created_object[i] = false;
                    }
                }

                spawn_timer += dt;

                if (attack_active && spawn_timer >= 0.3 && objects_spawned < 15) {
                    for (int i = 0; i < 15; i++) {
                        if (!created_object[i]) {
                            Mix_PlayChannel(-1, appear_sound, 0);
                            int random_object = randint(0, 5);
                            active_objects[i] = props[0][random_object];

                            active_objects[i].collision.x = randint(battle_box.x, (battle_box.x + battle_box.w));
                            active_objects[i].collision.y = battle_box.y;

                            objects_speed[i] = randint(100, 200);

                            created_object[i] = true;
                            objects_spawned++;
                            spawn_timer = 0.0;
                            break;
                        }
                    }
                }

                for (int i = 0; i < 15; i++) {
                    if (created_object[i]) {
                        active_objects[i].collision.y += (int)(objects_speed[i] * dt);

                        if ((active_objects[i].collision.y + active_objects[i].collision.h) >= (battle_box.y + battle_box.h)) {
                            Mix_PlayChannel(-1, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, &active_objects[i].collision)) {
                            Mix_PlayChannel(-1, hit_sound, 0);
                            *player_health -= damage;
                            *ivulnerable = true;
                            created_object[i] = false;
                            continue;
                        }

                        SDL_RenderCopyEx(render, active_objects[i].current, NULL, &active_objects[i].collision, 90, NULL, 0);
                    }
                }

                if (turn_timer >= 9.5) {
                    attack_active = false;

                    bool all_cleared = true;
                    for (int i = 0; i < 15; i++) {
                        if (created_object[i]) {
                            all_cleared = false;
                            break;
                        }
                    }

                    if (all_cleared) {
                        objects_spawned = 0;
                    }
                }
                break;

            case 2:
                if (!attack_active && turn_timer <= 8.0) {
                    attack_active = true;
                    spawn_timer = 0.0;
                    objects_spawned = 0;

                    for (int i = 0; i < 15; i++) {
                        created_object[i] = false;
                    }
                }

                spawn_timer += dt;

                if (attack_active && spawn_timer >= 0.75 && objects_spawned < 6) {
                    for (int i = 0; i < 6; i += 2) {
                        if (!created_object[i] && !created_object[i + 1]) {
                            Mix_PlayChannel(-1, appear_sound, 0);
                            int pair_type = choice(3, 0, 2, 4);

                            active_objects[i] = props[1][pair_type];
                            active_objects[i + 1] = props[1][pair_type + 1];

                            active_objects[i].collision.x = soul->collision.x - active_objects[i].collision.w - 80;
                            active_objects[i].collision.y = soul->collision.y;
                            active_objects[i + 1].collision.x = soul->collision.x + 80;
                            active_objects[i + 1].collision.y = soul->collision.y;
                            
                            objects_speed[i] = 100;
                            objects_speed[i + 1] = -100;

                            created_object[i] = true;
                            created_object[i + 1] = true;
                            objects_spawned += 2;
                            spawn_timer = 0.0;
                            break;
                        }
                    }
                }

                for (int i = 0; i < 6; i++) {
                    if (created_object[i]) {
                        active_objects[i].collision.x += (int)(objects_speed[i] * dt);

                        if (i % 2 == 0) {
                            if (created_object[i] && created_object[i + 1]) {
                                if ((active_objects[i].collision.x + active_objects[i].collision.w) > active_objects[i + 1].collision.x) {
                                    Mix_PlayChannel(-1, strike_sound, 0);
                                    created_object[i] = false;
                                    created_object[i + 1] = false;
                                    objects_spawned -= 2;
                                    continue;
                                }
                            }
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, &active_objects[i].collision)) {
                            Mix_PlayChannel(-1, hit_sound, 0);
                            *player_health -= damage;
                            *ivulnerable = true;

                            if (i % 2 == 0 && created_object[i + 1]) {
                                created_object[i + 1] = false;
                                objects_spawned--;
                            }
                            else if (i % 2 == 1 && created_object[i - 1]) {
                                created_object[i - 1] = false;
                                objects_spawned--;
                            }

                            created_object[i] = false;
                            objects_spawned--;
                            continue;
                        }

                        SDL_RenderCopyEx(render, active_objects[i].current, NULL, &active_objects[i].collision, 0, NULL, 0);
                    }
                }

                if (turn_timer >= 9.5) {
                    attack_active = false;
                    played_appear_sound = false;

                    bool all_cleared = true;
                    for (int i = 0; i < 15; i++) {
                        if (created_object[i]) {
                            all_cleared = false;
                            break;
                        }
                    }

                    if (all_cleared) {
                        objects_spawned = 0;
                    }
                }

                break;

            case 3:
                if (!attack_active && turn_timer <= 8.0) {
                    alpha_counter += dt * 300;
                    if (alpha_counter >= 255) alpha_counter = 255;
                    SDL_SetTextureAlphaMod(props[2][0].current, alpha_counter);
                    if (!played_appear_sound) {
                        Mix_PlayChannel(-1, appear_sound, 0);
                        played_appear_sound = true;
                    }

                    props[2][0].collision.x = (battle_box.x + (battle_box.w / 2)) - (props[2][0].collision.w / 2);
                    props[2][0].collision.y = battle_box.y + 10;

                    if (turn_timer >= 2.0) {
                        props[2][0].current = props[2][1].current;

                        attack_active = true;
                        spawn_timer = 0.0;
                        objects_spawned = 0;

                        for (int i = 0; i < 15; i++) {
                            created_object[i] = false;
                        }
                    }
                }

                spawn_timer += dt;

                if (attack_active && spawn_timer >= 0.5 && objects_spawned < 15) {
                    for (int i = 0; i < 15; i++) {
                        if (!created_object[i]) {
                            Mix_PlayChannel(-1, born_sound, 0);
                            active_objects[i] = props[2][2];

                            active_objects[i].collision.x = (props[2][0].collision.x + (props[2][0].collision.w / 2)) - (active_objects[i].collision.w / 2);
                            active_objects[i].collision.y = (props[2][0].collision.y + (props[2][0].collision.h / 2)) - (active_objects[i].collision.h / 2);

                            int target_x = soul->collision.x + soul->collision.w / 2;
                            int target_y = soul->collision.y + soul->collision.h / 2;
                            int start_x = active_objects[i].collision.x + active_objects[i].collision.w / 2;
                            int start_y = active_objects[i].collision.y + active_objects[i].collision.h / 2;

                            double angle_rad = atan2(target_y - start_y, target_x - start_x);
                            objects_speed[i] = 130;

                            vel_x[i] = cos(angle_rad) * objects_speed[i];
                            vel_y[i] = sin(angle_rad) * objects_speed[i];
                            angles[i] = angle_rad * (180.0 / M_PI);

                            created_object[i] = true;
                            objects_spawned++;
                            spawn_timer = 0.0;
                            break;
                        }
                    }
                }

                for (int i = 0; i < 15; i++) {
                    if (created_object[i]) {
                        static double animation_counter = 0.0;
                        static int frame = 0;
                        animation_counter += dt;

                        if (frame > 1) frame = 0;

                        if (animation_counter >= 0.1) {
                            if (frame == 0) {
                                active_objects[i].current = props[2][2].current;
                            }
                            else {
                                active_objects[i].current = props[2][3].current;
                            }
                            frame++;
                            animation_counter = 0.0;
                        }
                            
                        active_objects[i].collision.x += (int)(vel_x[i] * dt);
                        active_objects[i].collision.y += (int)(vel_y[i] * dt);

                        if (active_objects[i].collision.x < battle_box.x + 5 || active_objects[i].collision.x + active_objects[i].collision.w > battle_box.x + battle_box.w || active_objects[i].collision.y < battle_box.y || active_objects[i].collision.y + active_objects[i].collision.h > battle_box.y + battle_box.h) {
                            Mix_PlayChannel(-1, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, &active_objects[i].collision)) {
                            Mix_PlayChannel(-1, hit_sound, 0);
                            *player_health -= damage;
                            *ivulnerable = true;
                            created_object[i] = false;
                            continue;
                        }

                        SDL_RenderCopyEx(render, active_objects[i].current, NULL, &active_objects[i].collision, angles[i] + 90, NULL, 0);
                    }
                }

                if (turn_timer >= 9.5) {
                    attack_active = false;
                    alpha_counter = 0.0;

                    bool all_cleared = true;
                    for (int i = 0; i < 15; i++) {
                        if (created_object[i]) {
                            all_cleared = false;
                            break;
                        }
                    }

                    if (all_cleared) {
                        objects_spawned = 0;
                    }
                }

                SDL_RenderCopy(render, props[2][0].current, NULL, &props[2][0].collision);
                SDL_RenderCopy(render, soul->current, NULL, &soul->collision);

                break;

            default:
                break;
        }
    }
    else if (clear) {
        spawn_timer = 0.0;
        objects_spawned = 0;
        attack_active = false;
        alpha_counter = 0;

        for (int i = 0; i < 15; i ++) {
            created_object[i] = false;
            objects_speed[i] = 0.0;
            vel_x[i] = 0.0;
            vel_y[i] = 0.0;
            angles[i] = 0.0;
        }
    }

}
