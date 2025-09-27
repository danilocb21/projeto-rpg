#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

// TELA:
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// RENDERIZAÇÃO:
#define WINDOW_FLAGS (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define IMAGE_FLAGS (IMG_INIT_PNG)
#define MIXER_FLAGS (MIX_INIT_MP3 | MIX_INIT_OGG)

// QUANTIDADES:
#define COLLISION_QUANTITY 12
#define SURFACE_QUANTITY 13
#define DIR_COUNT 4

// LIMITES:
#define MAX_OBJECT_AMOUNT 200
#define MAX_DIALOGUE_CHAR 512
#define MAX_DIALOGUE_STR 20

// GRANDEZAS:
#define BASE_FONT_SIZE 24
#define BUBBLE_FONT_SIZE 14
#define SFX_VOLUME 40
#define MUSIC_VOLUME 30

// CANAIS:
#define DEFAULT_CHANNEL -1
#define MUSIC_CHANNEL 0
#define SFX_CHANNEL 2
#define DIALOGUE_CHANNEL 3

// TÍTULO:
#define GAME_TITLE "C-Tale: Meneghetti Vs Python"

// BASE DO JOGO:
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} Game;

// PERSONAGEM:
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    SDL_Rect interact_collision;
    double sprite_vel;
    const Uint8 *keystate;
    int facing;
    int counters[DIR_COUNT];
    int health;
    int strength;
} Character;

// OBJETO ESTÁTICO:
typedef struct {
    SDL_Texture* texture;
    SDL_Rect collision;
    int facing;
} Prop;

// PROJÉTIL DE PRECISÃO:
typedef struct {
    SDL_Texture* texture;
    SDL_FRect collision;
} Projectile;

// PARÂMETROS DE ANIMAÇÃO:
typedef struct {
    SDL_Texture **frames;
    double timer;
    int counter;
    int count;
} Animation;

// PARÂMETROS DE DIÁLOGO:
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

// EFEITO SONORO:
typedef struct {
    Mix_Chunk *sound;
    bool has_played;
} Sound;

// PACOTE DE RENDERIZAÇÃO:
typedef struct {
    SDL_Texture* texture;
    SDL_Rect *collisions;
} RenderItem;

// FRAME DE CUTSCENE:
typedef struct {
    SDL_Texture* image;
    Text* text;
    double duration;
} CutsceneFrame;

// ESTADO DE DESVANECIMENTO:
typedef struct {
    double timer;
    Uint8 alpha;
    bool fading_in;
} FadeState;

// DIREÇÕES DE FRENTE DE SPRITE:
enum direction { UP, DOWN, LEFT, RIGHT };
// ESTADOS DO JOGO:
enum game_states { TITLE_SCREEN, CUTSCENE, OPEN_WORLD, BATTLE_SCREEN, DEATH_SCREEN, FINAL_SCREEN };
// ESTADOS DO PLAYER:
enum player_states { IDLE, MOVABLE, DIALOGUE, PAUSE, DEAD, IN_BATTLE };
// PERSONAGENS DE FRAME DE DIÁLOGO:
enum characters { MENEGHETTI, MENEGHETTI_ANGRY, MENEGHETTI_SAD, PYTHON, NONE, BUBBLE };
// MAPA DE POSIÇÃO DO MENU DE BATALHA:
enum battle_buttons { FIGHT, ACT, ITEM, LEAVE };
// ESTADO DA BATALHA:
enum battle_states { ON_MENU, ON_FIGHT, ON_ACT, ON_ITEM, ON_LEAVE };
// TURNO DA BATALHA:
enum battle_turns { CHOICE_TURN, ATTACK_TURN, SOUL_TURN, ACT_TURN };

// FUNÇÃO DE INICIALIZAÇÃO:
bool sdl_initialize(Game *game);

// FUNÇÕES DE CARREGAMENTO:
SDL_Texture *create_texture(SDL_Renderer *render, const char *dir);
Mix_Chunk *create_chunk(const char *dir, int volume);
TTF_Font *create_font(const char *dir, int size);
SDL_Texture *create_text(SDL_Renderer *render, const char *utf8_char, TTF_Font *font, SDL_Color color);

// FUNÇÕES DE GAMEPLAY:
void create_dialogue(Character *player, SDL_Renderer *render, Text *text, int *player_state, int *game_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Sound *sound, Prop *bubble_speech);
void reset_dialogue(Text *text);
void python_attacks(SDL_Renderer *render, Prop *soul, SDL_Rect battle_box, int *player_health, int damage, int attack_index, bool *ivulnerable, Projectile **props, double dt, double turn_timer, Sound *sound, bool clear);
void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], double *anim_timer, double anim_interval, Sound *sound);
SDL_Texture *animate_sprite(Animation *anim, double dt, double cooldown, bool blink);
bool rects_intersect(SDL_Rect *a, SDL_Rect *b, SDL_FRect *c);
bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count);
static int surface_to_sound_index(int surface_index);
static int detect_surface(SDL_Rect *player, SDL_Rect surfaces[], int surface_count);
void update_reflection(Character *original, Character* reflection, Animation *animation);
void organize_items(Prop *text_items);

// FUNÇÕES DE REGISTRO DE OBJETOS:
static void track_texture(SDL_Texture *texture);
static bool already_tracked_texture(SDL_Texture *texture);
static void track_chunk(Mix_Chunk *chunk);
static bool already_tracked_chunk(Mix_Chunk *chunk);
static void track_font(TTF_Font *font);
static bool already_tracked_font(TTF_Font *font);

// FUNÇÕES DE LIMPEZA:
void game_cleanup(Game *game, int exit_status);
void clean_tracked_resources(void);

// FUNÇÕES AUXILIARES:
static int utf8_charlen(const char *s);
static int utf8_copy_char(const char *s, char *out);
int renderitem_cmp(const void *pa, const void *pb);
int randint(int min, int max);
int choice(int count, ...);

// RASTREADORES GLOBAIS:
static SDL_Texture **guarded_textures = NULL;
static int guarded_textures_count = 0;
static int guarded_textures_capacity = 0;

static Mix_Chunk **guarded_chunks = NULL;
static int guarded_chunks_count = 0;
static int guarded_chunks_capacity = 0;

static TTF_Font **guarded_fonts = NULL;
static int guarded_fonts_count = 0;
static int guarded_fonts_capacity = 0;

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    (void) argc;
    (void) argv;

    int game_state = CUTSCENE; // Lembrar de deixar CUTSCENE aqui na versão final.
    int player_state = MOVABLE;
    int battle_state = ON_MENU;

    Game game = {
        .renderer = NULL,
        .window = NULL,
    };

    if (sdl_initialize(&game))
        game_cleanup(&game, EXIT_FAILURE);

    SDL_bool running = SDL_TRUE;
    SDL_Event event;

    // FADES:
    FadeState cutscene_fade = {0.0, 0, true};
    FadeState open_world_fade = {0.0, 255, true};
    FadeState end_scene_fade = {0.0, 0, true};

    // CORES:
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {101, 107, 117, 255};

    // FONTES:
    TTF_Font* title_text_font = create_font("assets/fonts/PixelOperator-Bold.ttf", BASE_FONT_SIZE);
    TTF_Font* dialogue_text_font = create_font("assets/fonts/PixelOperator-Bold.ttf", BASE_FONT_SIZE);
    TTF_Font* battle_text_font = create_font("assets/fonts/PixelOperatorSC-Bold.ttf", BASE_FONT_SIZE);
    TTF_Font* bubble_text_font = create_font("assets/fonts/PixelOperator-Bold.ttf", BUBBLE_FONT_SIZE);

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

    Animation mr_python_animation[DIR_COUNT];
    mr_python_animation[UP].count = 1;
    mr_python_animation[UP].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[UP].count);
    mr_python_animation[UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-back-1.png");
    mr_python_animation[DOWN].count = 2;
    mr_python_animation[DOWN].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[DOWN].count);
    mr_python_animation[DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-front-1.png");
    mr_python_animation[DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-front-2.png");
    mr_python_animation[LEFT].count = 2;
    mr_python_animation[LEFT].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[LEFT].count);
    mr_python_animation[LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-left-1.png");
    mr_python_animation[LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-left-2.png");
    mr_python_animation[RIGHT].count = 2;
    mr_python_animation[RIGHT].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[RIGHT].count);
    mr_python_animation[RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-right-1.png");
    mr_python_animation[RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-right-2.png");

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
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };
    if (!python_dialogue.frames[0] || !python_dialogue.frames[1]) {
        fprintf(stderr, "Error loading animation sprite: %s", IMG_GetError());
        return 1;
    }

    Animation title_text_anim = {
        .frames = (SDL_Texture*[]){create_text(game.renderer, "APERTE ENTER PARA COMEÇAR", title_text_font, gray), NULL},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation lake_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/lake-1.png"), create_texture(game.renderer, "assets/sprites/scenario/lake-2.png"), create_texture(game.renderer, "assets/sprites/scenario/lake-3.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 3
    };

    Animation ocean_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/ocean-1.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-2.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-3.png"), create_texture(game.renderer, "assets/sprites/scenario/ocean-4.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 4
    };

    Animation sky_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/sky-1.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-2.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-3.png"), create_texture(game.renderer, "assets/sprites/scenario/sky-4.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 4
    };

    Animation sun_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/scenario/sun-1.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-2.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-3.png"), create_texture(game.renderer, "assets/sprites/scenario/sun-4.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 4
    };

    Animation soul_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/soul.png"), NULL},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation bar_attack_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/bar-attack-2.png"), create_texture(game.renderer, "assets/sprites/battle/bar-attack-1.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation slash_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/slash-1.png"), create_texture(game.renderer, "assets/sprites/battle/slash-2.png"), create_texture(game.renderer, "assets/sprites/battle/slash-3.png"), create_texture(game.renderer, "assets/sprites/battle/slash-4.png"), create_texture(game.renderer, "assets/sprites/battle/slash-5.png"), create_texture(game.renderer, "assets/sprites/battle/slash-6.png")},
        .timer = 0.0,
        .counter = 0,
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
        .strength = 10,
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
        .texture = create_texture(game.renderer, "assets/sprites/battle/python-torso.png"),
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h},
    };

    Prop mr_python_arms = {
        .texture = python_arms_animation.frames[0],
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h}
    };

    Prop mr_python_legs = {
        .texture = python_legs_animation.frames[0],
        .collision = {mr_python_head.collision.x, mr_python_head.collision.y, mr_python_head.collision.w, mr_python_head.collision.h}
    };

    Prop slash = {
        .texture = slash_animation.frames[0],
        .collision = {mr_python_torso.collision.x + (mr_python_torso.collision.w / 2) + 16, mr_python_torso.collision.y + 32, 32, 164},
    };

    Prop title = {
        .texture = create_texture(game.renderer, "assets/sprites/hud/logo-c-tale.png"),
        .collision = {(SCREEN_WIDTH / 2) - 290, (SCREEN_HEIGHT / 2) - 32, 580, 63}
    };

    Prop title_text = {
        .texture = title_text_anim.frames[0],
    };
    int title_text_width, title_text_height;
    SDL_QueryTexture(title_text.texture, NULL, NULL, &title_text_width, &title_text_height);
    title_text.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - (title_text_width / 2), SCREEN_HEIGHT - 100, title_text_width, title_text_height};

    Prop soul = {
        .texture = soul_animation.frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 10, 20, 20}
    };

    Prop soul_shattered = {
        .texture = create_texture(game.renderer, "assets/sprites/battle/soul-broken.png")
    };

    Prop mr_python = {
        .texture = mr_python_animation[DOWN].frames[0],
        .facing = DOWN
    };

    Prop python_van = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/python-van.png")
    };

    Prop civic = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/civic-left.png")
    };

    Prop palm_left = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/palm-head-left.png")
    };

    Prop palm_right = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/palm-head-right.png")
    };

    Prop lake = {
        .texture = lake_animation.frames[0],
    };

    Prop ocean = {
        .texture = ocean_animation.frames[0],
    };
    
    Prop sky = {
        .texture = sky_animation.frames[0],
    };

    Prop mountains = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/mountains.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };

    Prop sun = {
        .texture = sun_animation.frames[0]
    };

    Prop clouds = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/clouds.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, 155}
    };
    SDL_SetTextureAlphaMod(clouds.texture, 200);
    SDL_Rect clouds_clone = {clouds.collision.x - clouds.collision.w, 0, SCREEN_WIDTH * 2, 155};

    Prop mountains_back = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/mountains-back.png"),
        .collision = {0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };

    Prop bubble_speech = {
        .texture = create_texture(game.renderer, "assets/sprites/battle/text-bubble.png"),
    };

    SDL_Texture* fight_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-fight.png"), create_texture(game.renderer, "assets/sprites/hud/button-fight-select.png")};
    Prop button_fight = {
        .texture = fight_b_textures[1],
        .collision = {26, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* act_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-act.png"), create_texture(game.renderer, "assets/sprites/hud/button-act-select.png")};
    Prop button_act = {
        .texture = act_b_textures[0],
        .collision = {button_fight.collision.x + button_fight.collision.w + 26, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* item_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-item.png"), create_texture(game.renderer, "assets/sprites/hud/button-item-select.png")};
    Prop button_item = {
        .texture = item_b_textures[0],
        .collision = {button_act.collision.x + button_act.collision.w + 25, SCREEN_HEIGHT - 68, 128, 48}
    };

    SDL_Texture* leave_b_textures[] = {create_texture(game.renderer, "assets/sprites/hud/button-leave.png"), create_texture(game.renderer, "assets/sprites/hud/button-leave-select.png")};
    Prop button_leave = {
        .texture = leave_b_textures[0],
        .collision = {button_item.collision.x + button_item.collision.w + 25, SCREEN_HEIGHT - 68, 128, 48}
    };

    // PROPS DA BATALHA:
    int battle_text_width, battle_text_height;
    Prop battle_name = {
        .texture = create_text(game.renderer, "MENEGHETTI", battle_text_font, white),
    };
    SDL_QueryTexture(battle_name.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_name.collision = (SDL_Rect){button_fight.collision.x + 6, button_fight.collision.y - battle_text_height - 8, battle_text_width,battle_text_height};

    Prop battle_hp = {
        .texture = create_text(game.renderer, "HP", battle_text_font, white),
    };
    SDL_QueryTexture(battle_hp.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_hp.collision = (SDL_Rect){button_act.collision.x + 35, button_fight.collision.y - battle_text_height - 8, battle_text_width, battle_text_height};

    Prop battle_hp_amount = {
        .texture = create_text(game.renderer, "20/20", battle_text_font, white),
    };
    SDL_QueryTexture(battle_hp_amount.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    battle_hp_amount.collision = (SDL_Rect){button_act.collision.x + 140, button_fight.collision.y - battle_text_height - 8, battle_text_width, battle_text_height};

    Prop food_amount_text = {
        .texture = create_text(game.renderer, "4x", battle_text_font, white),
    };
    SDL_QueryTexture(food_amount_text.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    food_amount_text.collision = (SDL_Rect){0, 0, battle_text_width, battle_text_height};

    Prop text_attack_act = {
        .texture = create_text(game.renderer, "* Mr. Python", dialogue_text_font, white),
    };
    SDL_QueryTexture(text_attack_act.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_attack_act.collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};

    Prop text_item = {
        .texture = create_text(game.renderer, "* Picanha", dialogue_text_font, white)
    };
    SDL_QueryTexture(text_item.texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_item.collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};

    Prop text_act[3];
    text_act[0].texture = create_text(game.renderer, "* Examinar", dialogue_text_font, white);
    SDL_QueryTexture(text_act[0].texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[0].collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};
    text_act[1].texture = create_text(game.renderer, "* Insultar", dialogue_text_font, white);
    SDL_QueryTexture(text_act[1].texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[1].collision = (SDL_Rect){69, text_act[0].collision.y + text_act[0].collision.h + 10, battle_text_width, battle_text_height};
    text_act[2].texture = create_text(game.renderer, "* Explicar", dialogue_text_font, white);
    SDL_QueryTexture(text_act[2].texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_act[2].collision = (SDL_Rect){text_act[0].collision.x + text_act[0].collision.w + 100, text_act[0].collision.y, battle_text_width, battle_text_height};

    Prop text_leave[2];
    text_leave[0].texture = create_text(game.renderer, "* Poupar", dialogue_text_font, white);
    SDL_QueryTexture(text_leave[0].texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_leave[0].collision = (SDL_Rect){69, (SCREEN_HEIGHT / 2) + 25, battle_text_width, battle_text_height};
    text_leave[1].texture = create_text(game.renderer, "* Fugir", dialogue_text_font, white);
    SDL_QueryTexture(text_leave[1].texture, NULL, NULL, &battle_text_width, &battle_text_height);
    text_leave[1].collision = (SDL_Rect){69, text_leave[0].collision.y + text_leave[0].collision.h + 10, battle_text_width, battle_text_height};

    Prop bar_target = {
        .texture = create_texture(game.renderer, "assets/sprites/battle/bar-target.png"),
        .collision = {25, (SCREEN_HEIGHT / 2) + 5, SCREEN_WIDTH - 50, 122}
    };
    Prop bar_attack = {
        .texture = bar_attack_animation.frames[0],
        .collision = {bar_target.collision.x + 20, bar_target.collision.y + 2, 14, bar_target.collision.h - 4}
    };

    int attack_widths, attack_heights;
    Projectile command_rain[6];
    command_rain[0].texture = create_texture(game.renderer, "assets/sprites/battle/if.png");
    SDL_QueryTexture(command_rain[0].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    command_rain[1].texture = create_texture(game.renderer, "assets/sprites/battle/else.png");
    SDL_QueryTexture(command_rain[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    command_rain[2].texture = create_texture(game.renderer, "assets/sprites/battle/elif.png");
    SDL_QueryTexture(command_rain[2].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    command_rain[3].texture = create_texture(game.renderer, "assets/sprites/battle/input.png");
    SDL_QueryTexture(command_rain[3].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    command_rain[4].texture = create_texture(game.renderer, "assets/sprites/battle/print.png");
    SDL_QueryTexture(command_rain[4].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[4].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    command_rain[5].texture = create_texture(game.renderer, "assets/sprites/battle/in.png");
    SDL_QueryTexture(command_rain[5].texture, NULL, NULL, &attack_widths, &attack_heights);
    command_rain[5].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

    Projectile parenthesis_enclosure[6];
    parenthesis_enclosure[0].texture = create_texture(game.renderer, "assets/sprites/battle/brackets-1.png");
    SDL_QueryTexture(parenthesis_enclosure[0].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[1].texture = create_texture(game.renderer, "assets/sprites/battle/brackets-2.png");
    SDL_QueryTexture(parenthesis_enclosure[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[2].texture = create_texture(game.renderer, "assets/sprites/battle/key-1.png");
    SDL_QueryTexture(parenthesis_enclosure[2].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[3].texture = create_texture(game.renderer, "assets/sprites/battle/key-2.png");
    SDL_QueryTexture(parenthesis_enclosure[3].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[4].texture = create_texture(game.renderer, "assets/sprites/battle/parenthesis-1.png");
    SDL_QueryTexture(parenthesis_enclosure[4].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[4].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    parenthesis_enclosure[5].texture = create_texture(game.renderer, "assets/sprites/battle/parenthesis-2.png");
    SDL_QueryTexture(parenthesis_enclosure[5].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[5].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

    Projectile python_mother[4];
    python_mother[0].texture = python_mother_animation.frames[0];
    SDL_QueryTexture(python_mother[0].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[1].texture = python_mother_animation.frames[1];
    SDL_QueryTexture(python_mother[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[2].texture = python_baby_animation.frames[0];
    SDL_QueryTexture(python_mother[2].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[3].texture = python_baby_animation.frames[1];
    SDL_QueryTexture(python_mother[3].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

    Projectile *python_props[] = {command_rain, parenthesis_enclosure, python_mother};

    SDL_Texture* damage_numbers[] = {create_texture(game.renderer, "assets/sprites/battle/number-10.png"), create_texture(game.renderer, "assets/sprites/battle/number-20.png"), create_texture(game.renderer, "assets/sprites/battle/number-30.png"), create_texture(game.renderer, "assets/sprites/battle/number-40.png")};
    Prop damage;

    // SONS:
    Sound cutscene_music = {
        .sound = create_chunk("assets/sounds/soundtracks/the_story_of_a_hero.wav", MUSIC_VOLUME),
        .has_played = false
    };

    Sound battle_music = {
        .sound = create_chunk("assets/sounds/soundtracks/battle_against_abstraction.wav", MUSIC_VOLUME),
        .has_played = false
    };

    Sound ambience = {
        .sound = create_chunk("assets/sounds/sound_effects/in-game/ambient_sound.wav", MUSIC_VOLUME),
        .has_played = false
    };

    Sound walking_sounds[6];
    walking_sounds[0].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_grass.wav", SFX_VOLUME);
    walking_sounds[0].has_played = false;
    walking_sounds[1].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_concrete.wav", SFX_VOLUME);
    walking_sounds[1].has_played = false;
    walking_sounds[2].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_sand.wav", SFX_VOLUME);
    walking_sounds[2].has_played = false;
    walking_sounds[3].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_bridge.wav", SFX_VOLUME);
    walking_sounds[3].has_played = false;
    walking_sounds[4].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_wood.wav", SFX_VOLUME);
    walking_sounds[4].has_played = false;
    walking_sounds[5].sound = create_chunk("assets/sounds/sound_effects/in-game/walking_dirt.wav", SFX_VOLUME);
    walking_sounds[5].has_played = false;

    Sound battle_sounds[5];
    battle_sounds[0].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/damage_taken.wav", SFX_VOLUME);
    battle_sounds[0].has_played = false;
    battle_sounds[1].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/object_appears.wav", SFX_VOLUME);
    battle_sounds[1].has_played = false;
    battle_sounds[2].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/python_ejects.wav", SFX_VOLUME);
    battle_sounds[2].has_played = false;
    battle_sounds[3].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/slam.wav", SFX_VOLUME);
    battle_sounds[3].has_played = false;
    battle_sounds[4].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/strike_sound.wav", SFX_VOLUME);
    battle_sounds[4].has_played = false;

    Sound dialogue_voices[4];
    dialogue_voices[0].sound = create_chunk("assets/sounds/sound_effects/in-game/meneghetti_voice.wav", SFX_VOLUME);
    dialogue_voices[0].has_played = false;
    dialogue_voices[1].sound = create_chunk("assets/sounds/sound_effects/in-game/mr_python_voice.wav", SFX_VOLUME);
    dialogue_voices[1].has_played = false;
    dialogue_voices[2].sound = create_chunk("assets/sounds/sound_effects/in-game/text_sound.wav", SFX_VOLUME);
    dialogue_voices[2].has_played = false;
    dialogue_voices[3].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/text_battle.wav", SFX_VOLUME);
    dialogue_voices[3].has_played = false;

    Sound civic_engine = {
        .sound = create_chunk("assets/sounds/sound_effects/in-game/car_engine.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound civic_brake = {
        .sound = create_chunk("assets/sounds/sound_effects/in-game/car_brake.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound civic_door = {
        .sound = create_chunk("assets/sounds/sound_effects/in-game/car_door.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound title_sound = {
        .sound = create_chunk("assets/sounds/sound_effects/in-game/logo_sound.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound battle_appears = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/battle_appears.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound move_button = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/move_selection.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound click_button = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/select_sound.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound slash_sound = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/slash.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound enemy_hit_sound = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/enemy_hit.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound eat_sound = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/heal_sound.wav", SFX_VOLUME),
        .has_played = false
    };

    Sound soul_break_sound = {
        .sound = create_chunk("assets/sounds/sound_effects/battle-sounds/soul_shatter.wav", SFX_VOLUME),
        .has_played = false
    };

    // BASES DE TEXTO:
    Text py_dialogue = {
        .writings = {"* Há quanto tempo, Meneghetti.", "* Mr. Python...", "* Você veio até aqui batalhar contra mim?", "* Lembra o que aconteceu da última vez, não é?", "* Você e as outras linguagens de baixo nível nem me arranharam. Foi realmente estúpido.", "* Não vou cometer os mesmos erros do passado...", "* Você vai pagar pelo que fez com eles.", "* As linguagens de baixo nível ainda não morreram.", "* Eu ainda estou aqui para acabar com você.", "* Que peninha... Deve ser tão triste ser o último que restou.", "* Eu entendo a sua frustração.", "* Vamos acabar com isso para que você se junte a eles logo.", "* Venha, Mr. Python."},
        .on_frame = {PYTHON, MENEGHETTI, PYTHON, PYTHON, PYTHON, MENEGHETTI_SAD, MENEGHETTI_ANGRY, MENEGHETTI, MENEGHETTI_ANGRY, PYTHON, PYTHON, PYTHON, MENEGHETTI_ANGRY},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text py_dialogue_ad = {
        .writings = {"* Hm? Você conseguiu voltar?", "* Você é realmente duro na queda, Meneghetti. Devo admitir.", "* Eu ainda não desisti. Não pense que vai ser fácil.", "* Vamos ver se você vai ter a mesma sorte desta vez."},
        .on_frame = {PYTHON, PYTHON, MENEGHETTI_ANGRY, PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text py_dialogue_ad_2 = {
        .writings = {"* Que insistência. Por que não desiste logo?", "* Não enquanto eu não acabar com você.", "* Hahahah. Não precisa ser tão agressivo."},
        .on_frame = {PYTHON, MENEGHETTI_ANGRY, PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text py_dialogue_ad_3 = {
        .writings = {"* Acho que está um pouco difícil para você. Quer que eu diminua a dificuldade?", "* Cala a boca.", "* Desculpa, pessoal, eu tentei. Vamos para mais um round então."},
        .on_frame = {PYTHON, MENEGHETTI, PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text py_dialogue_ad_4 = {
        .writings = {"* ...", "* Vamos logo com isso."},
        .on_frame = {MENEGHETTI, PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text van_dialogue = {
        .writings = {"* Então este é o Python-móvel...", "* Agora tenho certeza de que meu inimigo está aqui..."},
        .on_frame = {MENEGHETTI, MENEGHETTI_ANGRY},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text lake_dialogue = {
        .writings = {"* O lago com animação te faz pensar sobre os esforços do criador deste universo.", "* Isso te enche de determinação.", "* Se fosse programado em Python não daria pra fazer isso."},
        .text_font = dialogue_text_font,
        .on_frame = {NONE, NONE, MENEGHETTI},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text arrival_dialogue = {
        .writings = {"* Meu radar-C detectou locomoções de alto nível por esta área.", "* Hora de acabar com isso de uma vez por todas."},
        .text_font = dialogue_text_font,
        .on_frame = {MENEGHETTI, MENEGHETTI},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text end_dialogue = {
        .writings = {"* Como... Como que isso foi acontecer?", "* Não faz sentido... Nós tínhamos ganhado essa luta.", "* EU já havia ganhado.", "* ...", "* Esse não é o fim, Meneghetti.", "* Por agora, você venceu. Mas um dia...", "* Um dia, as linguagens de baixo nível serão esquecidas.", "* E esse será o dia de sua ruína, e do meu triunfo.", "* Após anos de reinado das linguagens de alto nível...", "* A luz que um dia havia sumido dos programadores finalmente voltou a brilhar.", "* Um raio de esperança e um futuro próspero agora poderiam ser contemplados.", "* Tudo isso graças à ele..."},
        .text_font = dialogue_text_font,
        .on_frame = {PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, PYTHON, NONE, NONE, NONE, NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_1 = {
        .writings = {"Na época de ouro da computação, o mundo vivia em harmonia com diversas linguagens de programação."},
        .text_font = dialogue_text_font,
        .on_frame = {NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    
    Text cutscene_2 = {
        .writings = {"Porém, com os avanços tecnológicos, surgiu dependência e abstração na vida dos programadores."},
        .text_font = dialogue_text_font,
        .on_frame = {NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_3 = {
        .writings = {"No fim, restaram mínimos usuários de linguagens de baixo nível, o mundo fora tomado pela praticidade. Mas ainda havia resistência."},
        .text_font = dialogue_text_font,
        .on_frame = {NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text cutscene_4 = {
        .writings = {"Para trazer a luz para o mundo novamente, um dos heróis restantes lutará contra todas as abstrações e seu maior inimigo..."},
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = dialogue_text_font,
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
        .text_font = bubble_text_font,
        .on_frame = {BUBBLE},
        .text_color = black,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text bubble_speech_2 = {
        .writings = {"As linguagens de baixo nível já estão ultrapassadas."},
        .text_font = bubble_text_font,
        .on_frame = {BUBBLE},
        .text_color = black,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Text bubble_speech_3 = {
        .writings = {"Te darei um final digno."},
        .text_font = bubble_text_font,
        .on_frame = {BUBBLE},
        .text_color = black,
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

    SDL_Rect debug_buttons[] = {{25, 25, 25, 25}, {75, 25, 25, 25}, {125, 25, 25, 25}, {175, 25, 25, 25}, {225, 25, 25, 25}, {275, 25, 25, 25}};
    SDL_Rect animated_box;

    Uint32 last_ticks = SDL_GetTicks();
    double anim_timer = 0.0;
    const double anim_interval = 0.12;

    double cloud_timer = 0.0;

    int last_health = meneghetti.health;

    // VARIÁVEIS DE CONTROLE:
    bool delay_started = false; // Controla o estado da animação de chegada do player.
    bool battle_ready = false; // Indica se a animação de entrada de batalha terminou de acontecer.
    bool on_dialogue = false; // Cena de batalha, indica se um diálogo está passando na caixa.
    bool first_dialogue = false; // Serve para indicar se o diálog de entrada da batalha deve passar.
    bool animated_box_inited = false; // Indica se a caixa animada da batalha foi inicializada em relação à caixa padrão.
    bool soul_ivulnerable = false; // Indica se a alma está ou não ivulnerável.
    bool enemy_attack_selected = false; // Indica a necessidade de escolher o ataque inimigo no início do turno.
    bool tried_to_attack = false; // Indica se houve tentativa de ataque no teste de acurácia da batalha.
    bool should_expand_back = false; // Aponta se a caixa animada da batalha deve voltar ao seu estado original.
    bool first_insult = true; // Controla a quantidade de ações "Insulto" aplicadas pelo player.
    bool first_explain = true; // Controla a quantidade de ações "Explicar" aplicadas pelo player.
    bool python_dead = false; // Define se o Mr. Python foi morto ou não. Trigger para tela final.
    bool interaction_request = false; // Indica se foi requisitada uma interação via tecla E no mundo aberto.
    bool meneghetti_arrived = false; // Indica a chegada do player na posição necessária na intro do mundo aberto.
    bool python_dialogue_finished = false; // Aponta se o player finalizou seu primeiro diálogo com o Mr. Python. Trigger para a batalha.
    double pre_title_timer = 0.0; // Define a quantidade de tempo que o título permanece no início.
    bool pre_title = true;
    int cutscene_index = 0;
    double cutscene_timer = 0.0;
    bool last_frame_extend = false;
    bool debug_mode = false; // Indica o estado do modo de debug.

    double arrival_timer = 0.0;
    double car_animation_timer = 0.0;
    double senoidal_timer = 0.0;
    double counter = 0.0;
    double animated_shrink_timer = 0.0;
    double blink_timer = 0.0;
    double ivulnerability_counter = 0.0;
    double turn_timer = 0.0;
    double death_counter  = 0.0;

    float parallax_factor = 0.5f;

    int food_amount = 4;
    int selected_button = FIGHT;
    int turn = CHOICE_TURN;
    int menu_pos = 0;
    int base_py_damage = 2;
    int current_py_damage = base_py_damage;
    int death_count = 0;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = SDL_FALSE;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_E:
                    if (player_state == MOVABLE)
                        interaction_request = true;
                    break;
                case SDL_SCANCODE_F7:
                    if (debug_mode) {
                        debug_mode = false;
                    }
                    else {
                        debug_mode = true;
                    }
                default:
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT && debug_mode) {
                    SDL_Rect click = {event.button.x, event.button.y, 1, 1};

                    if (check_collision(&click, debug_buttons, 6)) {
                        for (int i = 0; i < 6; i++) {
                            if (rects_intersect(&click, &debug_buttons[i], NULL)) {
                                switch (i) {
                                case 0:
                                    // RESET PARA CUTSCENE:
                                    pre_title_timer = 0.0;
                                    pre_title = true;
                                    cutscene_index = 0;
                                    cutscene_timer = 0.0;
                                    cutscene_fade = (FadeState){0.0, 0, true};
                                    last_frame_extend = false;
                                    cutscene_music.has_played = false;

                                    meneghetti.facing = DOWN;

                                    game_state = CUTSCENE;
                                    player_state = IDLE;
                                    break;
                                case 1:
                                    // RESET PARA TITLE SCREEN:
                                    title_sound.has_played = false;

                                    open_world_fade.alpha = 255;
                                    open_world_fade.fading_in = true;
                                    open_world_fade.timer = 0.0;

                                    meneghetti_arrived = false;
                                    scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                                    meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                                    meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                                    meneghetti_civic.collision = (SDL_Rect){scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42};

                                    meneghetti.facing = DOWN;

                                    meneghetti_arrived = false;
                                    delay_started = false;
                                    car_animation_timer = 0.0;
                                    arrival_timer = 0.0;
                                    python_dialogue_finished = false;
                                    first_dialogue = false;
                                    ambience.has_played = false;

                                    game_state = TITLE_SCREEN;
                                    player_state = IDLE;
                                    break;
                                case 2:
                                    // RESET PARA OPEN WORLD:
                                    open_world_fade.alpha = 0;
                                    open_world_fade.fading_in = false;
                                    open_world_fade.timer = 0.0;

                                    scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                                    meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                                    meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};

                                    meneghetti_arrived = true;
                                    delay_started = false;
                                    car_animation_timer = 0.0;
                                    arrival_timer = 0.0;
                                    python_dialogue_finished = false;
                                    first_dialogue = false;
                                    battle_ready = false;
                                    ambience.has_played = false;
                                    battle_music.has_played = false;

                                    animated_shrink_timer = 0.0;
                                    blink_timer = 0.0;
                                    ivulnerability_counter = 0.0;
                                    turn_timer = 0.0;

                                    selected_button = FIGHT;
                                    turn = CHOICE_TURN;
                                    menu_pos = 0;
                                    current_py_damage = base_py_damage;
                                    food_amount = 4;
                                    counter = 0.0;
                                    senoidal_timer = 0.0;

                                    game_state = OPEN_WORLD;
                                    player_state = MOVABLE;
                                    break;
                                case 3:
                                    // RESET PARA BATTLE SCREEN:
                                    battle_ready = false;
                                    counter = 0.0;
                                    senoidal_timer = 0.0;
                                    on_dialogue = false;
                                    first_dialogue = false;
                                    animated_box_inited = false;
                                    soul_ivulnerable = false;
                                    enemy_attack_selected = false;
                                    tried_to_attack = false;
                                    should_expand_back = false;
                                    first_insult = true;
                                    first_explain = true;

                                    animated_shrink_timer = 0.0;
                                    blink_timer = 0.0;
                                    ivulnerability_counter = 0.0;
                                    turn_timer = 0.0;

                                    selected_button = FIGHT;
                                    turn = CHOICE_TURN;
                                    menu_pos = 0;
                                    current_py_damage = base_py_damage;
                                    food_amount = 4;

                                    bar_attack.collision.x = bar_target.collision.x + 20;

                                    mr_python_head.health = 200;
                                    mr_python_head.texture = python_head_animation.frames[0];
                                    mr_python_arms.texture = python_arms_animation.frames[0];
                                    mr_python_legs.texture = python_legs_animation.frames[0];

                                    soul_animation.counter = 0;
                                    slash_animation.counter = 0;
                                    bar_attack_animation.counter = 0;

                                    battle_sounds[0].has_played = false;
                                    battle_sounds[1].has_played = false;
                                    battle_sounds[2].has_played = false;
                                    battle_sounds[3].has_played = false;
                                    battle_sounds[4].has_played = false;
                                    slash_sound.has_played = false;
                                    enemy_hit_sound.has_played = false;
                                    eat_sound.has_played = false;
                                    battle_appears.has_played = false;

                                    python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, 0, &soul_ivulnerable, python_props, 0, 0, battle_sounds, true);

                                    game_state = BATTLE_SCREEN;
                                    player_state = IN_BATTLE;
                                    break;
                                case 4:
                                    // RESET PARA DEATH SCREEN:
                                    death_counter = 0.0;
                                    soul_break_sound.has_played = false;

                                    game_state = DEATH_SCREEN;
                                    player_state = DEAD;
                                    break;
                                case 5:
                                    // RESET PARA FINAL SCREEN:
                                    end_scene_fade.alpha = 0;
                                    end_scene_fade.fading_in = false;
                                    end_scene_fade.timer = 0.0;

                                    game_state = FINAL_SCREEN;
                                    player_state = IDLE;
                                    break;
                                default:
                                    break;
                                }

                                reset_dialogue(&cutscene_1);
                                reset_dialogue(&cutscene_2);
                                reset_dialogue(&cutscene_3);
                                reset_dialogue(&cutscene_4);
                                reset_dialogue(&py_dialogue);
                                reset_dialogue(&py_dialogue_ad);
                                reset_dialogue(&py_dialogue_ad_2);
                                reset_dialogue(&py_dialogue_ad_3);
                                reset_dialogue(&py_dialogue_ad_4);
                                reset_dialogue(&van_dialogue);
                                reset_dialogue(&lake_dialogue);
                                reset_dialogue(&arrival_dialogue);
                                reset_dialogue(&fight_start_txt);
                                reset_dialogue(&fight_generic_txt);
                                reset_dialogue(&fight_leave_txt);
                                reset_dialogue(&fight_spare_txt);
                                reset_dialogue(&fight_act_txt);
                                reset_dialogue(&insult_txt);
                                reset_dialogue(&insult_generic_txt);
                                reset_dialogue(&explain_txt);
                                reset_dialogue(&explain_generic_txt);
                                reset_dialogue(&picanha_txt);
                                reset_dialogue(&no_food_txt);
                                reset_dialogue(&bubble_speech_1);
                                reset_dialogue(&bubble_speech_2);
                                reset_dialogue(&bubble_speech_3);
                                reset_dialogue(&end_dialogue);

                                soul.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 10, 20, 20};
                                soul.texture = soul_animation.frames[0];
                            
                                interaction_request = false;
                                Mix_HaltChannel(MUSIC_CHANNEL);
                                Mix_HaltChannel(SFX_CHANNEL);
                            }
                        }
                    }
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

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 0);

            if (pre_title) {
                pre_title_timer += dt;
                
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, title.texture, NULL, &title.collision);

                if (pre_title_timer >= 5) {
                    pre_title = false;
                }
            }
            else {
                if (!cutscene_music.has_played) {
                    Mix_PlayChannel(MUSIC_CHANNEL, cutscene_music.sound, 0);
                    cutscene_music.has_played = true;
                }
                CutsceneFrame *current_frame = &cutscene[cutscene_index];
                cutscene_timer += dt;

                if (cutscene_fade.fading_in) {
                    cutscene_fade.timer += dt;
                    cutscene_fade.alpha = (Uint8)((cutscene_fade.timer / 1.0) * 255);
                    if (cutscene_fade.timer >= 1.0) {
                        cutscene_fade.alpha = 255;
                        cutscene_fade.fading_in = false;
                        cutscene_fade.timer = 0.0;
                    }
                }
                else if (cutscene_timer >= current_frame->duration - 1.0) {
                    cutscene_fade.timer += dt;
                    cutscene_fade.alpha = 255 - (Uint8)((cutscene_fade.timer / 1.0) * 255);
                    if (cutscene_fade.timer >= 1.0) {
                        cutscene_fade.alpha = 0;
                    }
                }

                if (cutscene_index == cutscene_amount - 1 && cutscene_timer >= current_frame->duration - 3.0 && !last_frame_extend) {
                    last_frame_extend = true;
                    cutscene_fade.timer = 0.0;
                }

                if (last_frame_extend) {
                    cutscene_fade.timer += dt;
                    cutscene_fade.alpha = 255 - (Uint8)((cutscene_fade.timer / 5.0) * 255);
                    if (cutscene_fade.timer >= 5.0) {
                        cutscene_fade.alpha = 0;
                    }
                }

                SDL_SetTextureAlphaMod(current_frame->image, cutscene_fade.alpha);
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, current_frame->image, NULL, NULL);

                if (current_frame->text) {
                    create_dialogue(&meneghetti, game.renderer, current_frame->text, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, false);
                }

                if (cutscene_timer >= current_frame->duration + 0.5 && !last_frame_extend) {
                    cutscene_timer = 0.0;
                    cutscene_index++;
                    cutscene_fade.fading_in = true;
                    cutscene_fade.timer = 0.0;
                    cutscene_fade.alpha = 0;

                    if (cutscene_index >= cutscene_amount) {
                        cutscene_index = cutscene_amount - 1;
                        last_frame_extend = true;
                        cutscene_fade.timer = 0.0;
                    }
                }

                if (interaction_request) {
                    game_state = TITLE_SCREEN;
                    Mix_HaltChannel(MUSIC_CHANNEL);
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                }
                if (last_frame_extend && cutscene_fade.timer >= 3.0) {
                    game_state = TITLE_SCREEN;
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                }
            }
            interaction_request = false;
        }

        if (game_state == TITLE_SCREEN) {
            const Uint8 *keys = meneghetti.keystate ? meneghetti.keystate : SDL_GetKeyboardState(NULL);

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);
            SDL_RenderCopy(game.renderer, title.texture, NULL, &title.collision);

            if (!title_sound.has_played) {
                Mix_PlayChannel(SFX_CHANNEL, title_sound.sound, 0);
                title_sound.has_played = true;
            }
            if (!Mix_Playing(SFX_CHANNEL)) {
                title_text.texture = animate_sprite(&title_text_anim, dt, 0.7, false);
                SDL_RenderCopy(game.renderer, title_text.texture, NULL, &title_text.collision);

                if (keys[SDL_SCANCODE_RETURN]) {
                    title_sound.has_played = false;
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
            if (!ambience.has_played) {
                Mix_PlayChannel(MUSIC_CHANNEL, ambience.sound, -1);
                ambience.has_played = true;
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
                if (rects_intersect(&meneghetti.interact_collision, &boxes[8], NULL))
                    player_state = DIALOGUE;

                if (rects_intersect(&meneghetti.interact_collision, &boxes[9], NULL))
                    player_state = DIALOGUE;
                
                if (rects_intersect(&meneghetti.interact_collision, &boxes[7], NULL))
                    player_state = DIALOGUE;

                interaction_request = false;
            }

            update_reflection(&meneghetti, &meneghetti_reflection, anim_pack_reflex);

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer); 

            SDL_RenderCopy(game.renderer, sky.texture, NULL, &sky.collision);
            SDL_RenderCopy(game.renderer, sun.texture, NULL, &sun.collision);
            SDL_RenderCopy(game.renderer, clouds.texture, NULL, &clouds.collision);
            SDL_RenderCopy(game.renderer, clouds.texture, NULL, &clouds_clone);
            SDL_RenderCopy(game.renderer, mountains_back.texture, NULL, &mountains_back.collision);
            SDL_RenderCopy(game.renderer, mountains.texture, NULL, &mountains.collision);
            SDL_RenderCopy(game.renderer, ocean.texture, NULL, &ocean.collision);
            SDL_RenderCopy(game.renderer, lake.texture, NULL, &lake.collision);
            SDL_RenderCopyEx(game.renderer, meneghetti_reflection.texture, NULL, &meneghetti_reflection.collision, 0, NULL, SDL_FLIP_VERTICAL);
            SDL_RenderCopy(game.renderer, scenario.texture, NULL, &scenario.collision);

            mr_python.texture = animate_sprite(&mr_python_animation[mr_python.facing], dt, 3.0, true);
            lake.texture = animate_sprite(&lake_animation, dt, 0.5, false);
            ocean.texture = animate_sprite(&ocean_animation, dt, 0.7, false);
            sky.texture = animate_sprite(&sky_animation, dt, 0.8, false);
            sun.texture = animate_sprite(&sun_animation, dt, 0.5, false);

            RenderItem items[2];
            int item_count = 0;

            items[item_count].texture = mr_python.texture;
            items[item_count].collisions = &mr_python.collision;
            item_count ++;

            items[item_count].texture = python_van.texture;
            items[item_count].collisions = &python_van.collision;
            item_count ++;
            
            if (meneghetti_arrived) {
                items[item_count].texture = civic.texture;
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
                if (rects_intersect (&meneghetti.interact_collision, &boxes[8], NULL)) {
                    switch (death_count) {
                        case 0:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                            break;
                        case 1:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                            break;
                        case 2:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_2, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                            break;
                        case 3:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_3, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                            break;
                        default:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_4, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                            break;
                    }
                    switch (meneghetti.facing) {
                        case UP:
                            mr_python.facing = DOWN;
                            break;
                        case DOWN:
                            mr_python.facing = UP;
                            break;
                        case LEFT:
                            mr_python.facing = RIGHT;
                            break;
                        case RIGHT:
                            mr_python.facing = LEFT;
                            break;
                        default:
                            break;
                    }
                    python_dialogue_finished = true;
                }

                if (rects_intersect (&meneghetti.interact_collision, &boxes[9], NULL))
                    create_dialogue(&meneghetti, game.renderer, &van_dialogue, &player_state, &game_state, dt, meneghetti_dialogue, &python_dialogue, &anim_timer, dialogue_voices, false);
                    
                if (rects_intersect (&meneghetti.interact_collision, &boxes[7], NULL))
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
                    if (!Mix_Playing(SFX_CHANNEL))
                        Mix_PlayChannel(SFX_CHANNEL, civic_engine.sound, 0);
                    
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    meneghetti_civic.collision.x -= 5;
                    meneghetti_civic.collision.y = (int)((scenario.collision.y + 731) + 2 * sin(car_animation_timer * 30.0)); 
                }
                else if (!delay_started) {
                    Mix_PlayChannel(SFX_CHANNEL, civic_brake.sound, 0);
                    
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    delay_started = true;
                    arrival_timer = 0.0;
                }
                else {
                    SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                    if (delay_started && !Mix_Playing(SFX_CHANNEL)) {
                        arrival_timer += dt;
                        if (arrival_timer >= 2.0) {
                            Mix_PlayChannel(SFX_CHANNEL, civic_door.sound, 0);

                            delay_started = false;
                            meneghetti_arrived = true;
                            player_state = DIALOGUE;
                            first_dialogue = true;
                            arrival_timer = 0.0;
                        }
                    }
                }
                SDL_RenderCopy(game.renderer, palm_left.texture, NULL, &palm_left.collision);
                SDL_RenderCopy(game.renderer, palm_right.texture, NULL, &palm_right.collision);
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

            char x_number[3];
            snprintf(x_number, sizeof(x_number), "%dx", food_amount);
            food_amount_text.texture = create_text(game.renderer, x_number, battle_text_font, white);

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

                SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                if (counter <= 0.5) {
                    if (!battle_appears.has_played) {
                        Mix_PlayChannel(SFX_CHANNEL, battle_appears.sound, 0);
                        battle_appears.has_played = true;
                    }
                    soul.texture = animate_sprite(&soul_animation, dt, 0.1, false);
                }
                else {
                    soul.texture = soul_animation.frames[0];
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
                        soul.texture = soul_animation.frames[0];
                        battle_appears.has_played = false;
                        battle_ready = true;
                        counter = 0.0;
                    }
                }
            }
            else {
                if (!battle_music.has_played) {
                    Mix_PlayChannel(MUSIC_CHANNEL, battle_music.sound, 0);
                    battle_music.has_played = true;
                }
                counter += dt;
                senoidal_timer += dt;
                if (counter >= 0.4) counter = 0.2;

                if (meneghetti.health != last_health) {
                    char hp_string[6];
                    snprintf(hp_string, sizeof(hp_string), "%02d/20", meneghetti.health);

                    battle_hp_amount.texture = create_text(game.renderer, hp_string, battle_text_font, white);

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

                SDL_RenderCopy(game.renderer, button_fight.texture, NULL, &button_fight.collision);
                SDL_RenderCopy(game.renderer, button_act.texture, NULL, &button_act.collision);
                SDL_RenderCopy(game.renderer, button_item.texture, NULL, &button_item.collision);
                SDL_RenderCopy(game.renderer, button_leave.texture, NULL, &button_leave.collision);
                SDL_RenderCopy(game.renderer, battle_name.texture, NULL, &battle_name.collision);
                SDL_RenderCopy(game.renderer, battle_hp.texture, NULL, &battle_hp.collision);
                SDL_RenderCopy(game.renderer, battle_hp_amount.texture, NULL, &battle_hp_amount.collision);

                // MR. PYTHON
                mr_python_head.collision.y = (int)(25 + 2 * sin(senoidal_timer * 1.5));
                mr_python_torso.collision.y = (int)(25 + 3 * sin(senoidal_timer * 1.5));
                mr_python_arms.collision.y = (int)(25 + 4 * sin(senoidal_timer * 1.5));

                SDL_RenderCopy(game.renderer, mr_python_arms.texture, NULL, &mr_python_arms.collision);
                SDL_RenderCopy(game.renderer, mr_python_legs.texture, NULL, &mr_python_legs.collision);
                SDL_RenderCopy(game.renderer, mr_python_torso.texture, NULL, &mr_python_torso.collision);
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
                        Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                        selected_button++;
                        counter = 0.0;
                    }
                    else if (keys[SDL_SCANCODE_A] && counter >= 0.2) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                        selected_button--;
                        counter = 0.0;
                    }

                    switch(selected_button) {
                        case FIGHT:
                            button_fight.texture = fight_b_textures[1];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case ACT:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[1];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case ITEM:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[1];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case LEAVE:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[1];
                            break;
                        default:
                            break;
                    }

                    if (e_just_pressed && counter >= 0.2) {
                        switch(selected_button) {
                            case FIGHT:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_FIGHT;
                                break;
                            case ACT:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_ACT;
                                break;
                            case ITEM:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_ITEM;
                                break;
                            case LEAVE:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
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

                        SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                        SDL_RenderCopy(game.renderer, text_attack_act.texture, NULL, &text_attack_act.collision);

                        if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                            battle_state = ON_MENU;
                            counter = 0.0;
                        }
                        if (e_just_pressed && counter >= 0.2) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                            turn = ATTACK_TURN;
                            counter = 0.0;
                        }
                    }

                    if (turn == ATTACK_TURN) {
                        int attack_damage;

                        static int bar_speed = 14;
                        SDL_RenderCopy(game.renderer, bar_target.texture, NULL, &bar_target.collision);
                        SDL_RenderCopy(game.renderer, bar_attack.texture, NULL, &bar_attack.collision);
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
                                
                                if (rects_intersect(&bar_attack.collision, &perfect_hit_rect, NULL)) {
                                    attack_damage = meneghetti.strength * 3;
                                    damage.texture = damage_numbers[3];
                                    SDL_QueryTexture(damage.texture, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &good_hit_rect, NULL)) {
                                    attack_damage = meneghetti.strength * 1.5;
                                    damage.texture = damage_numbers[2];
                                    SDL_QueryTexture(damage.texture, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &normal_hit_rect, NULL)) {
                                    attack_damage = meneghetti.strength;
                                    damage.texture = damage_numbers[1];
                                    SDL_QueryTexture(damage.texture, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                                else if (rects_intersect(&bar_attack.collision, &bad_hit_rect, NULL)) {
                                    attack_damage = meneghetti.strength * 0.5;
                                    damage.texture = damage_numbers[0];
                                    SDL_QueryTexture(damage.texture, NULL, NULL, &w, &h);
                                    damage.collision.w = w;
                                    damage.collision.h = h;
                                }
                            }

                            bar_attack.collision.x += bar_speed;
                        }
                        else {
                            blink_timer += dt;


                            if (blink_timer <= 3.0) {
                                bar_attack.texture = animate_sprite(&bar_attack_animation, dt, 0.1, false);

                                if (!slash_sound.has_played) {
                                    Mix_PlayChannel(SFX_CHANNEL, slash_sound.sound, 0);
                                    slash_sound.has_played = true;
                                }
                                SDL_RenderCopy(game.renderer, slash.texture, NULL, &slash.collision);
                                if (slash_animation.counter < 5) {
                                    slash.texture = animate_sprite(&slash_animation, dt, 0.2, false);
                                    if (slash_animation.counter > 3) {
                                        if (!enemy_hit_sound.has_played) {
                                            Mix_PlayChannel(SFX_CHANNEL, enemy_hit_sound.sound, 0);
                                            enemy_hit_sound.has_played = true;
                                            mr_python_head.health -= attack_damage;
                                        }
                                        SDL_RenderCopy(game.renderer, damage.texture, NULL, &damage.collision);
                                        damage.collision.y--;

                                        mr_python_head.texture = python_head_animation.frames[1];
                                        mr_python_arms.texture = python_arms_animation.frames[1];
                                        mr_python_legs.texture = python_legs_animation.frames[1];
                                        
                                        mr_python_head.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_torso.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_legs.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                        mr_python_arms.collision.x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(senoidal_timer * 40.0);
                                    }
                                }
                                else {
                                    slash.texture = NULL;
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

                                if (slash_animation.counter > 3) {
                                    SDL_RenderCopy(game.renderer, damage.texture, NULL, &damage.collision);
                                }
                            }   
                            else {
                                mr_python_head.texture = python_head_animation.frames[0];
                                mr_python_arms.texture = python_arms_animation.frames[0];
                                mr_python_legs.texture = python_legs_animation.frames[0];

                                slash_sound.has_played = false;
                                enemy_hit_sound.has_played = false;

                                blink_timer = 0.0;
                                tried_to_attack = false;
                                turn = SOUL_TURN;
                                slash_animation.counter = 0;
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

                            soul.texture = animate_sprite(&soul_animation, dt, 0.1, false);
                            if (ivulnerability_counter >= 1.0) {
                                soul.texture = soul_animation.frames[0];
                                soul_ivulnerable = false;
                                ivulnerability_counter = 0.0;
                            }
                        }

                        if (!should_expand_back && animated_box.w > target_w) {
                            animated_shrink_timer += dt;
                            if (animated_shrink_timer > 0.8) animated_shrink_timer = 0.8;

                            double t = animated_shrink_timer / 0.8;
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
                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);

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
                                    python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, enemy_attack, &soul_ivulnerable, python_props, dt, turn_timer, battle_sounds, false);
                                    switch (random_dialogue) {
                                        case 1: 
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_1, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, &bubble_speech);
                                            break;
                                        case 2:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_2, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, &bubble_speech);
                                            break;
                                        case 3:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_3, &player_state, &game_state, dt, NULL, NULL, &anim_timer, dialogue_voices, &bubble_speech);
                                            break;
                                        default:
                                            break;
                                    }   
                                }
                            }
                            else {
                                reset_dialogue(&bubble_speech_1);
                                reset_dialogue(&bubble_speech_2);
                                reset_dialogue(&bubble_speech_3);

                                python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, enemy_attack, &soul_ivulnerable, python_props, dt, turn_timer, battle_sounds, true);
                                python_props[2][0].texture = python_mother_animation.frames[0];
                                should_expand_back = true;
                                animated_shrink_timer = 0.0;
                                turn_timer = 0.0;
                            }
                        }
                        else if (should_expand_back) {
                            animated_shrink_timer += dt;
                            if (animated_shrink_timer > 0.8) animated_shrink_timer = 0.8;

                            double t = animated_shrink_timer / 0.8;
                            t = 1.0 - pow(1.0 - t, 3.0);

                            double start_w = (double)base_box.h;
                            double target_expand_w = (double)base_box.w;
                            double new_w = start_w + (target_expand_w - start_w) * t;

                            int center_x = base_box.x + base_box.w / 2;
                            animated_box.w = (int)(new_w + 0.5);
                            animated_box.x = center_x - animated_box.w / 2;
                            animated_box.y = base_box.y;
                            animated_box.h = base_box.h;

                            if (animated_shrink_timer >= 0.8) {
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

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_attack_act.texture, NULL, &text_attack_act.collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
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

                                SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                                SDL_RenderCopy(game.renderer, text_act[0].texture, NULL, &text_act[0].collision);
                                SDL_RenderCopy(game.renderer, text_act[1].texture, NULL, &text_act[1].collision);
                                SDL_RenderCopy(game.renderer, text_act[2].texture, NULL, &text_act[2].collision);

                                if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                    turn = CHOICE_TURN;
                                    counter = 0.0;
                                }
                                if (e_just_pressed && counter >= 0.2) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
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
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    menu_pos++;
                                    counter = 0.0;
                                }
                                if (keys[SDL_SCANCODE_W] && counter >= 0.2) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    menu_pos--;
                                    counter = 0.0;
                                }
                                if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_A]) {
                                if (counter >= 0.2) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
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
                        on_dialogue = false;
                        counter = 0.0;
                        menu_pos = 0;
                        if (food_amount > 0) food_amount--;

                        if (eat_sound.has_played) {
                            turn = SOUL_TURN;
                            battle_state = ON_FIGHT;
                            player_state = IN_BATTLE;
                        }
                        else {
                            turn = CHOICE_TURN;
                            battle_state = ON_MENU;
                            player_state = IN_BATTLE;
                        }

                        eat_sound.has_played = false;
                    }
                    else {
                        if (!on_dialogue && food_amount > 0) {
                            soul.collision.x = text_item.collision.x - soul.collision.w - 11;
                            soul.collision.y = text_item.collision.y + 2;
                            food_amount_text.collision.x = text_item.collision.x + text_item.collision.w + 5;
                            food_amount_text.collision.y = text_item.collision.y;

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_item.texture, NULL, &text_item.collision);
                            SDL_RenderCopy(game.renderer, food_amount_text.texture, NULL, &food_amount_text.collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                meneghetti.health += 20;
                                on_dialogue = true;
                                counter = 0.0;
                            }
                        }
                        else {
                            if (!eat_sound.has_played && food_amount > 0) {
                                Mix_PlayChannel(SFX_CHANNEL, eat_sound.sound, 0);
                                eat_sound.has_played = true;
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

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_leave[0].texture, NULL, &text_leave[0].collision);
                            SDL_RenderCopy(game.renderer, text_leave[1].texture, NULL, &text_leave[1].collision);

                            if (keys[SDL_SCANCODE_TAB] && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_state = ON_MENU;
                                counter = 0.0;
                            }
                            if (e_just_pressed && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                on_dialogue = true;
                                counter = 0.0;
                            }
                            if (keys[SDL_SCANCODE_S] && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                menu_pos++;
                                counter = 0.0;
                            }
                            if (keys[SDL_SCANCODE_W] && counter >= 0.2) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
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
                    mr_python_arms.texture = python_arms_animation.frames[1];
                    mr_python_legs.texture = python_legs_animation.frames[1];
                    
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
                Mix_HaltChannel(MUSIC_CHANNEL);

                mr_python_head.texture = python_head_animation.frames[0];
                mr_python_arms.texture = python_arms_animation.frames[0];
                mr_python_legs.texture = python_legs_animation.frames[0];

                python_props[2][0].texture = python_mother_animation.frames[0];
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
                slash_sound.has_played = false;
                enemy_hit_sound.has_played = false;
                soul_ivulnerable = false;
                ivulnerability_counter = 0.0;
                turn_timer = 0.0;
                should_expand_back = false;
                first_insult = true;
                first_explain = true;
                eat_sound.has_played = false;

                python_attacks(game.renderer, &soul, animated_box, &meneghetti.health, current_py_damage, 0, &soul_ivulnerable, python_props, dt, turn_timer, battle_sounds, true);

                int attack_widths, attack_heights;
                SDL_QueryTexture(command_rain[0].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(command_rain[1].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(command_rain[2].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(command_rain[3].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(command_rain[4].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[4].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(command_rain[5].texture, NULL, NULL, &attack_widths, &attack_heights);
                command_rain[5].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

                SDL_QueryTexture(parenthesis_enclosure[0].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(parenthesis_enclosure[1].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(parenthesis_enclosure[2].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(parenthesis_enclosure[3].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(parenthesis_enclosure[4].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[4].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                SDL_QueryTexture(parenthesis_enclosure[5].texture, NULL, NULL, &attack_widths, &attack_heights);
                parenthesis_enclosure[5].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

                python_mother[0].texture = python_mother_animation.frames[0];
                SDL_QueryTexture(python_mother[0].texture, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                python_mother[1].texture = python_mother_animation.frames[1];
                SDL_QueryTexture(python_mother[1].texture, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                python_mother[2].texture = python_baby_animation.frames[0];
                SDL_QueryTexture(python_mother[2].texture, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
                python_mother[3].texture = python_baby_animation.frames[1];
                SDL_QueryTexture(python_mother[3].texture, NULL, NULL, &attack_widths, &attack_heights);
                python_mother[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};

                python_props[0] = command_rain;
                python_props[1] = parenthesis_enclosure;
                python_props[2] = python_mother;

                soul_animation.counter = 0;
                slash_animation.counter = 0;
                mr_python_head.health = 200;
                bar_attack.collision.x = bar_target.collision.x + 20;

                if (player_state == DEAD) {
                    game_state = DEATH_SCREEN;
                    death_count++;
                }
                else if (python_dead) {
                    game_state = FINAL_SCREEN;
                    death_count = 0;
                    end_scene_fade.timer = 0.0;
                    end_scene_fade.alpha = 255;
                    end_scene_fade.fading_in = true;
                    python_dead = false;
                }
            }
        }

        if (game_state == DEATH_SCREEN) {
            death_counter += dt;

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            if (death_counter <= 2.0) {
                if (!soul_break_sound.has_played) {
                    Mix_PlayChannel(SFX_CHANNEL, soul_break_sound.sound, 0);
                    soul_break_sound.has_played = true;
                }
                SDL_RenderCopy(game.renderer, soul_shattered.texture, NULL, &soul.collision);
            }
            else {
                open_world_fade.alpha = (Uint8)255;
                open_world_fade.fading_in = true;
                open_world_fade.timer = 0.0;
                death_counter = 0.0;

                soul_break_sound.has_played = false;
                meneghetti_arrived = true;
                player_state = MOVABLE;
                game_state = OPEN_WORLD;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                meneghetti_civic.collision = (SDL_Rect){scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42};
                
                delay_started = false;
                car_animation_timer = 0.0;
                arrival_timer = 0.0;
                python_dialogue_finished = false;
                first_dialogue = false;
                ambience.has_played = false;
                battle_music.has_played = false;

                meneghetti.health = 20;
                last_health = meneghetti.health;
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

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

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

                soul_break_sound.has_played = false;
                meneghetti_arrived = false;
                player_state = IDLE;
                game_state = TITLE_SCREEN;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                meneghetti_civic.collision = (SDL_Rect){scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42};
                
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

        if (debug_mode) {
            SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
            SDL_RenderFillRects(game.renderer, debug_buttons, 6);
        }

        SDL_RenderPresent(game.renderer);

        SDL_Delay(1);
    }

    for (int i = 0; i < DIR_COUNT; i++) {
        free(anim_pack[i].frames);
        free(anim_pack_reflex[i].frames);
        free(mr_python_animation[i].frames);
    }
    for (int i = 0; i < 3; i++) {
        free(meneghetti_dialogue[i].frames);
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

SDL_Texture* create_texture(SDL_Renderer *render, const char *dir) {
    SDL_Surface* surface = IMG_Load(dir);
    if (!surface) {
        fprintf(stderr, "Error loading image '%s': %s\n", dir, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    if (!texture) {
        fprintf(stderr, "Error creating texture: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }

    SDL_FreeSurface(surface);
    track_texture(texture);
    return texture;
}

Mix_Chunk* create_chunk(const char *dir, int volume) {
    Mix_Chunk* chunk = Mix_LoadWAV(dir);

    if (!chunk) {
        fprintf(stderr, "Error loading chunk %s: %s", dir, Mix_GetError());
        return NULL;
    }

    Mix_VolumeChunk(chunk, volume);
    track_chunk(chunk);
    return chunk;
}

TTF_Font* create_font(const char *dir, int size) {
    TTF_Font* font = TTF_OpenFont(dir, size);

    if (!font) {
        fprintf(stderr, "Error loading font %s: %s", dir, TTF_GetError());
        return NULL;
    }

    track_font(font);
    return font;
}

SDL_Texture* create_text(SDL_Renderer *render, const char *utf8_text, TTF_Font *font, SDL_Color color) {
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
    track_texture(texture);
    return texture;
}

void create_dialogue(Character *player, SDL_Renderer *render, Text *text, int *player_state, int *game_state, double dt, Animation *meneghetti_face, Animation *python_face, double *anim_timer, Sound *sound, Prop *bubble_speech) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    
    bool has_meneghetti = (meneghetti_face != NULL);
    bool has_python = (python_face != NULL);
    bool bubble;
    
    static double anim_cooldown = 0.2;

    static double e_cooldown = 0.0;
    static bool prev_e_pressed = false;
    e_cooldown += dt;
    bool e_now = keys[SDL_SCANCODE_E];

    static int counters[] = {0, 0};
    
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
    if (bubble_speech) {
        bubble_speech->collision = dialogue_box;
        bubble = true;
    }
    else {
        bubble = false;
    }
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
                SDL_QueryTexture(bubble_speech->texture, NULL, NULL, &w, &h);
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
        SDL_RenderCopy(render, bubble_speech->texture, NULL, &dialogue_box);
    }

    // BORDAS:
    if (*game_state != CUTSCENE && *game_state != BATTLE_SCREEN && *game_state != FINAL_SCREEN) {
        SDL_Rect box_borders[] = {{dialogue_box.x, dialogue_box.y, dialogue_box.w, 5}, {dialogue_box.x, dialogue_box.y, 5, dialogue_box.h}, {dialogue_box.x, dialogue_box.y + dialogue_box.h - 5, dialogue_box.w, 5}, {dialogue_box.x + dialogue_box.w - 5, dialogue_box.y, 5, dialogue_box.h}};
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderFillRects(render, box_borders, 4);
    }

    SDL_Rect meneghetti_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 72, 96};
    SDL_Rect python_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 96, 96};

    if (text->on_frame[text->cur_str] != NONE && text->on_frame[text->cur_str] != BUBBLE) {
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

    static int last_cur_str = -1;
    static double sfx_timer = 0.0;
    const double sfx_cooldown = 0.03;
    sfx_timer += dt;

    if (text->cur_str == 0 && text->cur_byte == 0) {
        e_cooldown = 0.0;
        counters[0] = 0;
        counters[1] = 0;
        last_cur_str = -1;
    }

    if (text->cur_str != last_cur_str) {
        e_cooldown = 0.0;
        counters[0] = 0;
        counters[1] = 0;
        last_cur_str = text->cur_str;
    }

    if (!text->waiting_for_input) {
        *anim_timer += dt;
        text->timer += dt;
        if (e_pressed && *game_state != BATTLE_SCREEN && !bubble) {
            const char *current = text->writings[text->cur_str];
            while (current[text->cur_byte] != '\0' && text->char_count < MAX_DIALOGUE_CHAR) {
                char utf8_buffer[5];
                int n = utf8_copy_char(&current[text->cur_byte], utf8_buffer);
                SDL_Texture* t = create_text(render, utf8_buffer, text->text_font, text->text_color);
                if (t) {
                    text->chars[text->char_count] = t;
                    strcpy(text->chars_string[text->char_count], utf8_buffer);
                    text->char_count++;
                }
                text->cur_byte += n;
            }
            if(!bubble) text->waiting_for_input = true;
        }
        else if (text->timer >= timer_delay) {
            text->timer = 0.0;
            const char *current = text->writings[text->cur_str];
            
            if (current[text->cur_byte] == '\0') {
               if (!bubble) text->waiting_for_input = true;
            }
            else {
                if (text->char_count < MAX_DIALOGUE_CHAR) {
                    char utf8_buffer[5];
                    int n = utf8_copy_char(&current[text->cur_byte], utf8_buffer);
                    SDL_Texture* t = create_text(render, utf8_buffer, text->text_font, text->text_color);
                    if (t) {
                        if (sound && sfx_timer >= sfx_cooldown) {
                            int speaker = text->on_frame[text->cur_str];
                            Mix_Chunk* chunk = NULL;
                            
                            if (speaker == MENEGHETTI || speaker == MENEGHETTI_ANGRY || speaker ==  MENEGHETTI_SAD) {
                                chunk = sound[0].sound;
                            }
                            if (speaker == PYTHON) {
                                chunk = sound[1].sound;
                            }
                            if (speaker == NONE) {
                                chunk = sound[2].sound;
                            }
                            if (speaker == BUBBLE) {
                                chunk = sound[3].sound;
                            }

                            if (chunk) {
                                Mix_PlayChannel(DIALOGUE_CHANNEL, chunk, 0);
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
            if (!bubble) text->waiting_for_input = true;
            break;
        }
    }

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

void python_attacks(SDL_Renderer *render, Prop *soul, SDL_Rect battle_box, int *player_health, int damage, int attack_index, bool *ivulnerable, Projectile **props, double dt, double turn_timer, Sound *sound, bool clear) {
    static double spawn_timer = 0.0;
    static int objects_spawned = 0;
    static bool attack_active = false;
    
    static Projectile active_objects[15];
    static bool created_object[15] = {false};
    static double objects_speed[15] = {0};

    static double vel_x[15] = {0};
    static double vel_y[15] = {0};
    static double angles[15] = {0};

    static int alpha_counter = 0;

    static bool played_appear_sound = false;

    Mix_Chunk* hit_sound = sound[0].sound;
    Mix_Chunk* appear_sound = sound[1].sound;
    Mix_Chunk* born_sound = sound[2].sound;
    Mix_Chunk* slam_sound = sound[3].sound;
    Mix_Chunk* strike_sound = sound[4].sound;

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
                            Mix_PlayChannel(DEFAULT_CHANNEL, appear_sound, 0);
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
                        active_objects[i].collision.y += objects_speed[i] * dt;

                        if ((active_objects[i].collision.y + active_objects[i].collision.h) >= (battle_box.y + battle_box.h)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                            *player_health -= damage;
                            *ivulnerable = true;
                            created_object[i] = false;
                            continue;
                        }

                        SDL_RenderCopyExF(render, active_objects[i].texture, NULL, &active_objects[i].collision, 90, NULL, 0);
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
                            Mix_PlayChannel(DEFAULT_CHANNEL, appear_sound, 0);
                            int pair_type = choice(3, 0, 2, 4);

                            active_objects[i] = props[1][pair_type];
                            active_objects[i + 1] = props[1][pair_type + 1];

                            active_objects[i].collision.x = soul->collision.x - active_objects[i].collision.w - 80;
                            active_objects[i].collision.y = soul->collision.y;
                            active_objects[i + 1].collision.x = soul->collision.x + 80;
                            active_objects[i + 1].collision.y = soul->collision.y;
                            
                            objects_speed[i] = 130;
                            objects_speed[i + 1] = -130;

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
                        active_objects[i].collision.x += objects_speed[i] * dt;

                        if (i % 2 == 0) {
                            if (created_object[i] && created_object[i + 1]) {
                                if ((active_objects[i].collision.x + active_objects[i].collision.w) > active_objects[i + 1].collision.x) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, strike_sound, 0);
                                    created_object[i] = false;
                                    created_object[i + 1] = false;
                                    objects_spawned -= 2;
                                    continue;
                                }
                            }
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
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

                        SDL_RenderCopyExF(render, active_objects[i].texture, NULL, &active_objects[i].collision, 0, NULL, 0);
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
                    SDL_SetTextureAlphaMod(props[2][0].texture, alpha_counter);
                    if (!played_appear_sound) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, appear_sound, 0);
                        played_appear_sound = true;
                    }

                    props[2][0].collision.x = (battle_box.x + (battle_box.w / 2)) - (props[2][0].collision.w / 2);
                    props[2][0].collision.y = battle_box.y + 10;

                    if (turn_timer >= 2.0) {
                        props[2][0].texture = props[2][1].texture;

                        attack_active = true;
                        spawn_timer = 0.0;
                        objects_spawned = 0;

                        for (int i = 0; i < 15; i++) {
                            created_object[i] = false;
                        }
                    }
                }

                SDL_RenderCopyF(render, props[2][0].texture, NULL, &props[2][0].collision);
                SDL_RenderCopy(render, soul->texture, NULL, &soul->collision);

                spawn_timer += dt;

                if (attack_active && spawn_timer >= 0.5 && objects_spawned < 15) {
                    for (int i = 0; i < 15; i++) {
                        if (!created_object[i]) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, born_sound, 0);
                            active_objects[i] = props[2][2];

                            active_objects[i].collision.x = (props[2][0].collision.x + (props[2][0].collision.w / 2)) - (active_objects[i].collision.w / 2);
                            active_objects[i].collision.y = (props[2][0].collision.y + (props[2][0].collision.h / 2)) - (active_objects[i].collision.h / 2);

                            int target_x = soul->collision.x + (soul->collision.w / 2);
                            int target_y = soul->collision.y + (soul->collision.h / 2);
                            int start_x = active_objects[i].collision.x + (active_objects[i].collision.w / 2);
                            int start_y = active_objects[i].collision.y + (active_objects[i].collision.h / 2);

                            double angle_rad = atan2(target_y - start_y, target_x - start_x);
                            objects_speed[i] = 100;

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
                                active_objects[i].texture = props[2][2].texture;
                            }
                            else {
                                active_objects[i].texture = props[2][3].texture;
                            }
                            frame++;
                            animation_counter = 0.0;
                        }
                            
                        active_objects[i].collision.x += vel_x[i] * dt;
                        active_objects[i].collision.y += vel_y[i] * dt;

                        if (active_objects[i].collision.x < battle_box.x + 5 || active_objects[i].collision.x + active_objects[i].collision.w > battle_box.x + battle_box.w || active_objects[i].collision.y < battle_box.y || active_objects[i].collision.y + active_objects[i].collision.h > battle_box.y + battle_box.h) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!*ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                            *player_health -= damage;
                            *ivulnerable = true;
                            created_object[i] = false;
                            continue;
                        }

                        SDL_RenderCopyExF(render, active_objects[i].texture, NULL, &active_objects[i].collision, angles[i] + 90, NULL, 0);
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

void sprite_update(Character *scenario, Character *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], double *anim_timer, double anim_interval, Sound *sound) {
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
            if (Mix_Playing(SFX_CHANNEL)) {
                Mix_HaltChannel(SFX_CHANNEL);
            }

            if (new_sound_index != -1) {
                Mix_PlayChannel(SFX_CHANNEL, sound[new_sound_index].sound, -1);
            }
        }
        current_walk_sound = new_sound_index;
    }
    else {
        if (Mix_Playing(SFX_CHANNEL)) {
            Mix_HaltChannel(SFX_CHANNEL);
        }

        current_walk_sound = -1;
    }
}

SDL_Texture *animate_sprite(Animation *anim, double dt, double cooldown, bool blink) {
    if (!anim || anim->count <= 0) return NULL;

    if (cooldown <= 0.0) {
        anim->counter = (anim->counter + 1) % anim->count;
        anim->timer = 0.0;
        return anim->frames[anim->counter];
    }

    anim->timer += dt;

    int steps = (int)(anim->timer / cooldown);
    if (steps > 0) {
        anim->counter = (anim->counter + steps) % anim->count;
        anim->timer -= (double)steps * cooldown;
    }

    if (blink && anim->count == 2) {
        if (anim->counter == 1) {
            double blink_duration = cooldown / 7.0;
            if (anim->timer >= blink_duration) {
                anim->counter = 0;
            }
        }
        else {
            anim->counter = 0;
        }
    }

    anim->counter %= anim->count;
    return anim->frames[anim->counter];
}

bool rects_intersect(SDL_Rect *a, SDL_Rect *b, SDL_FRect *c) {
    if (b) {
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
    else if (c) {
        int leftX_A, leftX_C;
        int topY_A, topY_C;
        int rightX_A, rightX_C;
        int bottomY_A, bottomY_C;

        leftX_A = a->x;
        topY_A = a->y;
        rightX_A = a->x + a->w;
        bottomY_A = a->y + a->h;

        leftX_C = c->x;
        topY_C = c->y;
        rightX_C = c->x + c->w;
        bottomY_C = c->y + c->h;

        if (leftX_A >= rightX_C)
            return false;

        if (topY_A >= bottomY_C)
            return false;

        if (rightX_A <= leftX_C)
            return false;

        if (bottomY_A <= topY_C)
            return false;

        return true;
    }

    return false;
}

bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count) {
    for (int i = 0; i < box_count; i++) {
        if (rects_intersect(player, &boxes[i], NULL)) return true;
    }

    return false;
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
        if (text_items[i].texture != NULL) available++;
    }
    if (available == 4) return;

    int x_col1 = 69;
    int x_col2 = 69 + 200;

    int base_y = -1;
    for (int i = 0; i < 4; i++) {
        if (text_items[i].texture != NULL) {
            base_y = (SCREEN_HEIGHT / 2) + 25;
            break;
        }
    }
    if (base_y == -1) return;
    
    Prop *col1[2] = {NULL, NULL};
    int col1_count = 0;

    Prop *col2[2] = {NULL, NULL};
    int col2_count = 0;

    if (text_items[0].texture != NULL) col1[col1_count++] = &text_items[0];
    if (text_items[1].texture != NULL) col1[col1_count++] = &text_items[1];
    if (text_items[2].texture != NULL) col2[col2_count++] = &text_items[2];
    if (text_items[3].texture != NULL) col2[col2_count++] = &text_items[3];

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
        tmp[i].texture = NULL;
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

static void track_texture(SDL_Texture *texture) {
    if (!texture || already_tracked_texture(texture)) {
        return;
    }

    if (guarded_textures_count >= guarded_textures_capacity) {
        guarded_textures_capacity = guarded_textures_capacity ? guarded_textures_capacity * 2 : 64;
        guarded_textures = realloc(guarded_textures, guarded_textures_capacity * sizeof(*guarded_textures));
    }

    guarded_textures[guarded_textures_count++] = texture;
}

static bool already_tracked_texture(SDL_Texture *texture) {
    for (int i = 0; i < guarded_textures_count; i++) {
        if (guarded_textures[i] == texture) {
            return true;
        }
    }

    return false;
}

static void track_chunk(Mix_Chunk *chunk) {
    if (!chunk || already_tracked_chunk(chunk)) {
        return;
    }

    if (guarded_chunks_count >= guarded_chunks_capacity) {
        guarded_chunks_capacity = guarded_chunks_capacity ? guarded_chunks_capacity * 2 : 64;
        guarded_chunks = realloc(guarded_chunks, guarded_chunks_capacity * sizeof(*guarded_chunks));
    }

    guarded_chunks[guarded_chunks_count++] = chunk;
}

static bool already_tracked_chunk(Mix_Chunk *chunk) {
    for (int i = 0; i < guarded_chunks_count; i++) {
        if (guarded_chunks[i] == chunk) {
            return true;
        }
    }

    return false;
}

static void track_font(TTF_Font *font) {
    if (!font || already_tracked_font(font)) {
        return;
    }

    if (guarded_fonts_count >= guarded_fonts_capacity) {
        guarded_fonts_capacity = guarded_fonts_capacity ? guarded_fonts_capacity * 2 : 32;
        guarded_fonts = realloc(guarded_fonts, guarded_fonts_capacity * sizeof(*guarded_fonts));
    }

    guarded_fonts[guarded_fonts_count++] = font;
}

static bool already_tracked_font(TTF_Font *font) {
    for (int i = 0; i < guarded_fonts_count; i++) {
        if (guarded_fonts[i] == font) {
            return true;
        }
    }

    return false;
}

void game_cleanup(Game *game, int exit_status) {
    Mix_HaltMusic();
    
    for (int i = -1; i <= MIX_CHANNELS; i++) {
        Mix_HaltChannel(i);
    }

    Mix_CloseAudio();

    clean_tracked_resources();
    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);

    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    exit(exit_status);
}

void clean_tracked_resources(void) {
    for (int i = 0; i < guarded_textures_count; i++) {
        if (guarded_textures[i]) {
            SDL_DestroyTexture(guarded_textures[i]);
        }
    }
    free(guarded_textures);
    guarded_textures = NULL;
    guarded_textures_count = guarded_textures_capacity = 0;

    for (int i = 0; i < guarded_chunks_count; i++) {
        if (guarded_chunks[i]) {
            Mix_FreeChunk(guarded_chunks[i]);
        }
    }
    free(guarded_chunks);
    guarded_chunks = NULL;
    guarded_chunks_count = guarded_chunks_capacity = 0;

    for (int i = 0; i < guarded_fonts_count; i++) {
        if (guarded_fonts[i]) {
            TTF_CloseFont(guarded_fonts[i]);
        }
    }
    free(guarded_fonts);
    guarded_fonts = NULL;
    guarded_fonts_count = guarded_fonts_capacity = 0;
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
