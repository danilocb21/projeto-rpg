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
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include "get_username.h"

// TELA:
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// RENDERIZAÇÃO:
#define WINDOW_FLAGS (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define IMAGE_FLAGS (IMG_INIT_PNG)
#define MIXER_FLAGS (MIX_INIT_MP3 | MIX_INIT_OGG)

// QUANTIDADES:
#define SURFACE_QUANTITY 13
#define COLLISION_QUANTITY 13
#define DIRECTION_AMOUNT 4
#define DIALOGUE_AMOUNT 27
#define ENEMY_AMOUNT 1
#define NPC_AMOUNT 2
#define SOUND_AMOUNT 14
#define ENEMY_STATES 4
#define ENEMY_PARTS 4
#define FACE_AMOUNT 5
#define INPUT_DELAY 0.2

// LIMITES:
#define MAX_DIALOGUE_CHAR 200
#define MAX_DIALOGUE_SETS 15
#define MAX_DIALOGUE_STR 20
#define MAX_CUTSCENE_FRAMES 25
#define MAX_INVENTORY_SIZE 10

// GRANDEZAS:
#define BASE_FONT_SIZE 24
#define BUBBLE_FONT_SIZE 14
#define SFX_VOLUME 45
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
    int game_state;
    int last_game_state;
    bool debug_mode;
    bool player_on_scene;
} Game;

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
} Dialogue;

// OBJETO ESTÁTICO:
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    Animation animation;
} Prop;

// ITEM:
typedef struct {
    Prop *item_text;
    Prop *item_amount_text;
    int item_type;
    int attack_additional;
    int health_additional;
    int cure_additional;
    int amount;
} Item;

// JOGADOR:
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    SDL_Rect interact_collision;
    Item *inventory[MAX_INVENTORY_SIZE];
    int inventory_counter;
    const Uint8 *keystate;
    double input_timer;
    double dialogue_input_timer;
    double speed;
    int counters[DIRECTION_AMOUNT];
    int facing;
    int base_health;
    int last_health;
    int health;
    int base_strength;
    int strength;
    int player_state;
    int death_count;
} Player;

// INIMIGO:
typedef struct {
    SDL_Texture ***textures;
    int animation_status;
    SDL_Rect collision[ENEMY_PARTS];
    int texture_amount;
    int base_health;
    int health;
    int base_strength;
    int strength;
} Enemy;

// ALMA:
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    double ivulnerability_timer;
    bool is_ivulnerable;
} Soul;

// NPC:
typedef struct {
    SDL_Texture *texture;
    SDL_Rect collision;
    Dialogue *dialogues[MAX_DIALOGUE_SETS];
    int facing;
    int times_interacted;
    int dialogue_amount;
    bool was_interacted;
} NPC;

// PROJÉTIL DE PRECISÃO:
typedef struct {
    SDL_Texture *texture;
    SDL_FRect collision;
    Animation animation;
} Projectile;

// CAIXA DE BATALHA:
typedef struct {
    const SDL_Rect base_box;
    SDL_Rect animated_box;
    bool should_retract;
    double animation_timer;
} BattleBox;

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

// FRAME DE CUTSCENE_SCREEN:
typedef struct {
    SDL_Texture* image;
    Dialogue* text;
    double elapsed_time;
    double duration;
    bool extend_frame;
} CutsceneFrame;

typedef struct {
    CutsceneFrame *frames[MAX_CUTSCENE_FRAMES];
    int current_frame;
    int frame_amount;
} Cutscene;

// ESTADO DE DESVANECIMENTO:
typedef struct {
    double timer;
    Uint8 alpha;
    bool fading_in;
} FadeState;

// CONTROLE DE MENU:
typedef struct {
    int line;
    int column;
} MenuPosition;

// ESTADOS DE JOGO:
typedef struct {
    MenuPosition menu_position;
    int battle_state;
    int battle_turn;
    int selected_button;
    int turn_counter;
    bool reading_text;
    bool random_attack_selected;
    bool player_attacked;
    bool enemy_dead;
} BattleState;

typedef struct {
    double global_timer;
    double senoidal_timer;
    double cutscene_timer;
    double battle_timer;
    double turn_timer;
    double attack_timer;
    double death_timer;
} GameTimers;

// DIREÇÕES DE SPRITE:
enum direction { DIRECTION_UP, DIRECTION_DOWN, DIRECTION_LEFT, DIRECTION_RIGHT };
// ESTADOS DO JOGO:
enum game_states { CUTSCENE_SCREEN, TITLE_SCREEN, OPEN_WORLD_SCREEN, BATTLE_SCREEN, DEATH_SCREEN, FINAL_SCREEN };
// ESTADOS DO PLAYER:
enum player_states { PLAYER_IDLE, PLAYER_MOVABLE, PLAYER_ON_DIALOGUE, PLAYER_ON_PAUSE, PLAYER_DEAD, PLAYER_ON_BATTLE };
// PERSONAGENS DE FRAME DE DIÁLOGO:
enum characters { FACE_MENEGHETTI, FACE_MENEGHETTI_ANGRY, FACE_MENEGHETTI_SAD, FACE_PYTHON, FACE_CHATGPT, FACE_NONE, FACE_BUBBLE };
// MAPA DE POSIÇÃO DO MENU DE BATALHA:
enum battle_buttons { BUTTON_FIGHT, BUTTON_ACT, BUTTON_ITEM, BUTTON_LEAVE };
// ESTADO DA BATALHA:
enum battle_states { BATTLE_MENU, BATTLE_FIGHT, BATTLE_ACT, BATTLE_ITEM, BATTLE_LEAVE };
// TURNO DA BATALHA:
enum battle_turns { CHOICE_TURN, ATTACK_TURN, SOUL_TURN, ACT_TURN };
// IDENTIFICADORES DE TEXTURA DE INIMIGO:
enum enemy_textures { ENEMY_IDLE, ENEMY_VARIATION, ENEMY_HURT, ENEMY_DEAD };
enum enemy_parts { ENEMY_ARMS, ENEMY_LEGS, ENEMY_HEAD, ENEMY_TORSO };
// IDENTIFICADORES DE ITENS:
enum item_types { ITEM_FOOD, ITEM_WEAPON, ITEM_ARMOR };

// FUNÇÃO DE INICIALIZAÇÃO:
bool sdl_initialize(Game *game);

// FUNÇÃO DE RESET PARA O ESTADO DO GAME:
void game_reset(Game *game, GameTimers *timers, BattleState *battle, BattleBox *battle_box, Soul *soul, Player *player, Enemy *enemies[], NPC *npcs[], Dialogue *dialogues[], Sound *sounds[]);

// FUNÇÕES DE CARREGAMENTO:
SDL_Texture *create_texture(SDL_Renderer *render, const char *dir);
Mix_Chunk *create_chunk(const char *dir, int volume);
TTF_Font *create_font(const char *dir, int size);
SDL_Texture *create_text(SDL_Renderer *render, const char *utf8_char, TTF_Font *font, SDL_Color color);

// FUNÇÕES DE GAMEPLAY:
void create_dialogue(Player *player, SDL_Renderer *render, Dialogue *text, NPC *npc, int *player_state, int *game_state, double dt, Animation *dialogue_faces, Sound *sound, Prop *bubble_speech);
void reset_dialogue(Dialogue *text);
void python_attacks(SDL_Renderer *render, Soul *soul, BattleBox battle_box, int *player_health, int damage, int attack_index, Projectile **props, double dt, double turn_timer, Sound *sound, bool clear);
void sprite_update(Prop *scenario, Player *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], Sound *sound);
SDL_Texture *animate_sprite(Animation *anim, double dt, double cooldown, bool blink);
bool rects_intersect(SDL_Rect *a, SDL_Rect *b, SDL_FRect *c);
bool check_collision(SDL_Rect *player, SDL_Rect boxes[], int box_count);
static int surface_to_sound_index(int surface_index);
static int detect_surface(SDL_Rect *player, SDL_Rect surfaces[], int surface_count);
void update_reflection(Player *original, Player* reflection, Animation *animation);

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
char *utf8_to_upper(const char *s);
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

    Game game = {
        .renderer = NULL,
        .window = NULL,
        .game_state = CUTSCENE_SCREEN,
        .debug_mode = false,
        .player_on_scene = true
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
    Animation anim_pack[DIRECTION_AMOUNT];
    anim_pack[DIRECTION_UP].count = 3;
    anim_pack[DIRECTION_UP].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DIRECTION_UP].count);
    anim_pack[DIRECTION_UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back.png");
    anim_pack[DIRECTION_UP].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-1.png");
    anim_pack[DIRECTION_UP].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-2.png");
    anim_pack[DIRECTION_DOWN].count = 3;
    anim_pack[DIRECTION_DOWN].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DIRECTION_DOWN].count);
    anim_pack[DIRECTION_DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front.png");
    anim_pack[DIRECTION_DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-1.png");
    anim_pack[DIRECTION_DOWN].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-2.png");
    anim_pack[DIRECTION_LEFT].count = 2;
    anim_pack[DIRECTION_LEFT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DIRECTION_LEFT].count);
    anim_pack[DIRECTION_LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left.png");
    anim_pack[DIRECTION_LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left-1.png");
    anim_pack[DIRECTION_RIGHT].count = 2;
    anim_pack[DIRECTION_RIGHT].frames = malloc(sizeof(SDL_Texture*) * anim_pack[DIRECTION_RIGHT].count);
    anim_pack[DIRECTION_RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right.png");
    anim_pack[DIRECTION_RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right-1.png");
    anim_pack->timer = 0.0;

    Animation anim_pack_reflex[DIRECTION_AMOUNT];
    anim_pack_reflex[DIRECTION_UP].count = 3;
    anim_pack_reflex[DIRECTION_UP].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[DIRECTION_UP].count);
    anim_pack_reflex[DIRECTION_UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back.png");
    anim_pack_reflex[DIRECTION_UP].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-1.png");
    anim_pack_reflex[DIRECTION_UP].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-back-2.png");
    anim_pack_reflex[DIRECTION_DOWN].count = 3;
    anim_pack_reflex[DIRECTION_DOWN].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[DIRECTION_DOWN].count);
    anim_pack_reflex[DIRECTION_DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front.png");
    anim_pack_reflex[DIRECTION_DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-1.png");
    anim_pack_reflex[DIRECTION_DOWN].frames[2] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-front-2.png");
    anim_pack_reflex[DIRECTION_LEFT].count = 2;
    anim_pack_reflex[DIRECTION_LEFT].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[DIRECTION_LEFT].count);
    anim_pack_reflex[DIRECTION_LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left.png");
    anim_pack_reflex[DIRECTION_LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-left-1.png");
    anim_pack_reflex[DIRECTION_RIGHT].count = 2;
    anim_pack_reflex[DIRECTION_RIGHT].frames = malloc(sizeof(SDL_Texture*) * anim_pack_reflex[DIRECTION_RIGHT].count);
    anim_pack_reflex[DIRECTION_RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right.png");
    anim_pack_reflex[DIRECTION_RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-right-1.png");
    for (int i = 0; i < DIRECTION_AMOUNT; i++) {
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

    Animation mr_python_animation[DIRECTION_AMOUNT];
    mr_python_animation[DIRECTION_UP].count = 1;
    mr_python_animation[DIRECTION_UP].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[DIRECTION_UP].count);
    mr_python_animation[DIRECTION_UP].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-back-1.png");
    mr_python_animation[DIRECTION_DOWN].count = 2;
    mr_python_animation[DIRECTION_DOWN].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[DIRECTION_DOWN].count);
    mr_python_animation[DIRECTION_DOWN].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-front-1.png");
    mr_python_animation[DIRECTION_DOWN].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-front-2.png");
    mr_python_animation[DIRECTION_LEFT].count = 2;
    mr_python_animation[DIRECTION_LEFT].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[DIRECTION_LEFT].count);
    mr_python_animation[DIRECTION_LEFT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-left-1.png");
    mr_python_animation[DIRECTION_LEFT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-left-2.png");
    mr_python_animation[DIRECTION_RIGHT].count = 2;
    mr_python_animation[DIRECTION_RIGHT].frames = malloc(sizeof(SDL_Texture*) * mr_python_animation[DIRECTION_RIGHT].count);
    mr_python_animation[DIRECTION_RIGHT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/mr-python-right-1.png");
    mr_python_animation[DIRECTION_RIGHT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/mr-python-right-2.png");

    Animation chatgpt_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/characters/chatgpt-front-1.png"), create_texture(game.renderer, "assets/sprites/characters/chatgpt-front-2.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation dialogue_faces[FACE_AMOUNT];
    dialogue_faces[FACE_MENEGHETTI].count = 2;
    dialogue_faces[FACE_MENEGHETTI].counter = 0;
    dialogue_faces[FACE_MENEGHETTI].timer = 0.0;
    dialogue_faces[FACE_MENEGHETTI].frames = malloc(sizeof(SDL_Texture*) * dialogue_faces[FACE_MENEGHETTI].count);
    dialogue_faces[FACE_MENEGHETTI].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-1.png");
    dialogue_faces[FACE_MENEGHETTI].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-2.png");
    dialogue_faces[FACE_MENEGHETTI_ANGRY].count = 2;
    dialogue_faces[FACE_MENEGHETTI_ANGRY].counter = 0;
    dialogue_faces[FACE_MENEGHETTI_ANGRY].timer = 0.0;
    dialogue_faces[FACE_MENEGHETTI_ANGRY].frames = malloc(sizeof(SDL_Texture*) * dialogue_faces[FACE_MENEGHETTI_ANGRY].count);
    dialogue_faces[FACE_MENEGHETTI_ANGRY].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-1.png");
    dialogue_faces[FACE_MENEGHETTI_ANGRY].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-angry-2.png");
    dialogue_faces[FACE_MENEGHETTI_SAD].count = 2;
    dialogue_faces[FACE_MENEGHETTI_SAD].counter = 0;
    dialogue_faces[FACE_MENEGHETTI_SAD].timer = 0.0;
    dialogue_faces[FACE_MENEGHETTI_SAD].frames = malloc(sizeof(SDL_Texture*) * dialogue_faces[FACE_MENEGHETTI_SAD].count);
    dialogue_faces[FACE_MENEGHETTI_SAD].frames[0] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-sad-1.png");
    dialogue_faces[FACE_MENEGHETTI_SAD].frames[1] = create_texture(game.renderer, "assets/sprites/characters/meneghetti-dialogue-sad-2.png");
    dialogue_faces[FACE_PYTHON].count = 2;
    dialogue_faces[FACE_PYTHON].counter = 0;
    dialogue_faces[FACE_PYTHON].timer = 0.0;
    dialogue_faces[FACE_PYTHON].frames = malloc(sizeof(SDL_Texture*) * dialogue_faces[FACE_PYTHON].count);
    dialogue_faces[FACE_PYTHON].frames[0] = create_texture(game.renderer, "assets/sprites/characters/python-dialogue-1.png");
    dialogue_faces[FACE_PYTHON].frames[1] = create_texture(game.renderer, "assets/sprites/characters/python-dialogue-2.png");
    dialogue_faces[FACE_CHATGPT].count = 2;
    dialogue_faces[FACE_CHATGPT].counter = 0;
    dialogue_faces[FACE_CHATGPT].timer = 0.0;
    dialogue_faces[FACE_CHATGPT].frames = malloc(sizeof(SDL_Texture*) * dialogue_faces[FACE_CHATGPT].count);
    dialogue_faces[FACE_CHATGPT].frames[0] = create_texture(game.renderer, "assets/sprites/characters/chatgpt-dialogue-1.png");
    dialogue_faces[FACE_CHATGPT].frames[1] = create_texture(game.renderer, "assets/sprites/characters/chatgpt-dialogue-2.png");

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

    Animation python_mother_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-2.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation python_baby_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-baby-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-baby-2.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation python_barrier_left_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-barrier-left-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-barrier-left-2.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    Animation python_barrier_right_animation = {
        .frames = (SDL_Texture*[]){create_texture(game.renderer, "assets/sprites/battle/python-barrier-right-1.png"), create_texture(game.renderer, "assets/sprites/battle/python-barrier-right-2.png")},
        .timer = 0.0,
        .counter = 0,
        .count = 2
    };

    // OBJETOS:
    Player meneghetti = {
        .texture = anim_pack[DIRECTION_DOWN].frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32},
        .speed = 100.0f,
        .keystate = SDL_GetKeyboardState(NULL),
        .input_timer = 0.0,
        .dialogue_input_timer = 0.0,
        .interact_collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25},
        .base_health = 20,
        .last_health = 20,
        .health = 20,
        .strength = 10,
        .base_strength = 10,
        .facing = DIRECTION_DOWN,
        .counters = {0, 0, 0, 0},
        .death_count = 0,
        .inventory_counter = 0,
    };
    Player meneghetti_reflection = {
        .texture = anim_pack_reflex[DIRECTION_DOWN].frames[0],
        .facing = DIRECTION_DOWN,
        .counters = {0, 0, 0, 0}
    };

    Enemy mr_python = {
        .texture_amount = ENEMY_STATES,
        .animation_status = ENEMY_IDLE,
        .collision = {{(SCREEN_WIDTH / 2) - 102, 25, 204, 204}, {(SCREEN_WIDTH / 2) - 102, 25, 204, 204}, {(SCREEN_WIDTH / 2) - 102, 25, 204, 204}, {(SCREEN_WIDTH / 2) - 102, 25, 204, 204}},
        .health = 200,
        .base_health = 200,
        .strength = 2,
        .base_strength = 2
    };
    mr_python.textures = malloc(sizeof(SDL_Texture**) * ENEMY_STATES);
    for (int i = 0; i < ENEMY_STATES; i++) {
        mr_python.textures[i] = malloc(sizeof(SDL_Texture**) * ENEMY_PARTS);
    }
    mr_python.textures[ENEMY_IDLE][ENEMY_ARMS] = create_texture(game.renderer, "assets/sprites/battle/python-arms.png");
    mr_python.textures[ENEMY_IDLE][ENEMY_LEGS] = create_texture(game.renderer, "assets/sprites/battle/python-legs.png");
    mr_python.textures[ENEMY_IDLE][ENEMY_HEAD] = create_texture(game.renderer, "assets/sprites/battle/python-head-1.png");
    mr_python.textures[ENEMY_IDLE][ENEMY_TORSO] = create_texture(game.renderer, "assets/sprites/battle/python-torso.png");
    mr_python.textures[ENEMY_HURT][ENEMY_ARMS] = create_texture(game.renderer, "assets/sprites/battle/python-arms-hurt.png");
    mr_python.textures[ENEMY_HURT][ENEMY_LEGS] = create_texture(game.renderer, "assets/sprites/battle/python-legs-hurt.png");
    mr_python.textures[ENEMY_HURT][ENEMY_HEAD] = create_texture(game.renderer, "assets/sprites/battle/python-head-2.png");
    mr_python.textures[ENEMY_HURT][ENEMY_TORSO] = create_texture(game.renderer, "assets/sprites/battle/python-torso.png");

    Soul soul = {
        .texture = soul_animation.frames[0],
        .collision = {(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 10, 20, 20},
        .is_ivulnerable = false,
        .ivulnerability_timer = 0.0
    };

    Prop scenario = {
        .texture = create_texture(game.renderer, "assets/sprites/scenario/scenario.png"),
        .collision = {0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2}
    };

    Prop meneghetti_civic = {
        .texture = create_texture(game.renderer, "assets/sprites/characters/meneghetti-civic-left.png"),
        .collision = {scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42}
    };

    Prop slash = {
        .texture = slash_animation.frames[0],
        .collision = {mr_python.collision[0].x + (mr_python.collision[0].w / 2) + 16, mr_python.collision[0].y + 32, 32, 164},
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

    Prop soul_shattered = {
        .texture = create_texture(game.renderer, "assets/sprites/battle/soul-broken.png")
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
    parenthesis_enclosure[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};
    parenthesis_enclosure[1].texture = create_texture(game.renderer, "assets/sprites/battle/brackets-2.png");
    SDL_QueryTexture(parenthesis_enclosure[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};
    parenthesis_enclosure[2].texture = create_texture(game.renderer, "assets/sprites/battle/key-1.png");
    SDL_QueryTexture(parenthesis_enclosure[2].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};
    parenthesis_enclosure[3].texture = create_texture(game.renderer, "assets/sprites/battle/key-2.png");
    SDL_QueryTexture(parenthesis_enclosure[3].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[3].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};
    parenthesis_enclosure[4].texture = create_texture(game.renderer, "assets/sprites/battle/parenthesis-1.png");
    SDL_QueryTexture(parenthesis_enclosure[4].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[4].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};
    parenthesis_enclosure[5].texture = create_texture(game.renderer, "assets/sprites/battle/parenthesis-2.png");
    SDL_QueryTexture(parenthesis_enclosure[5].texture, NULL, NULL, &attack_widths, &attack_heights);
    parenthesis_enclosure[5].collision = (SDL_FRect){0, 0, attack_widths, attack_heights * 2};

    Projectile python_mother[3];
    python_mother[0].texture = python_mother_animation.frames[0];
    SDL_QueryTexture(python_mother[0].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[1].texture = python_mother_animation.frames[1];
    SDL_QueryTexture(python_mother[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[2].texture = python_baby_animation.frames[0];
    SDL_QueryTexture(python_mother[2].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_mother[2].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_mother[2].animation = python_baby_animation;

    Projectile python_barrier[2];
    python_barrier[0].texture = python_barrier_left_animation.frames[0];
    SDL_QueryTexture(python_barrier[0].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_barrier[0].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_barrier[0].animation = python_barrier_left_animation;
    python_barrier[1].texture = python_barrier_right_animation.frames[1];
    SDL_QueryTexture(python_barrier[1].texture, NULL, NULL, &attack_widths, &attack_heights);
    python_barrier[1].collision = (SDL_FRect){0, 0, attack_widths, attack_heights};
    python_barrier[1].animation = python_barrier_right_animation;

    Projectile *python_props[] = {command_rain, parenthesis_enclosure, python_mother, python_barrier};

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

    Sound dialogue_voices[5];
    dialogue_voices[0].sound = create_chunk("assets/sounds/sound_effects/in-game/meneghetti_voice.wav", SFX_VOLUME);
    dialogue_voices[0].has_played = false;
    dialogue_voices[1].sound = create_chunk("assets/sounds/sound_effects/in-game/mr_python_voice.wav", SFX_VOLUME);
    dialogue_voices[1].has_played = false;
    dialogue_voices[2].sound = create_chunk("assets/sounds/sound_effects/in-game/text_sound.wav", SFX_VOLUME);
    dialogue_voices[2].has_played = false;
    dialogue_voices[3].sound = create_chunk("assets/sounds/sound_effects/battle-sounds/text_battle.wav", SFX_VOLUME);
    dialogue_voices[3].has_played = false;
    dialogue_voices[4].sound = create_chunk("assets/sounds/sound_effects/in-game/chatgpt_voice.wav", SFX_VOLUME);
    dialogue_voices[4].has_played = false;

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
    Dialogue py_dialogue = {
        .writings = {"* Há quanto tempo, Meneghetti.", "* Mr. Python...", "* Você veio até aqui batalhar contra mim?", "* Lembra o que aconteceu da última vez, não é?", "* Você e as outras linguagens de baixo nível nem me arranharam. Foi realmente estúpido.", "* Não vou cometer os mesmos erros do passado...", "* Você vai pagar pelo que fez com eles.", "* As linguagens de baixo nível ainda não morreram.", "* Eu ainda estou aqui para acabar com você.", "* Que peninha... Deve ser tão triste ser o último que restou.", "* Eu entendo a sua frustração.", "* Vamos acabar com isso para que você se junte a eles logo.", "* Venha, Mr. Python."},
        .on_frame = {FACE_PYTHON, FACE_MENEGHETTI, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_MENEGHETTI_SAD, FACE_MENEGHETTI_ANGRY, FACE_MENEGHETTI, FACE_MENEGHETTI_ANGRY, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_MENEGHETTI_ANGRY},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue py_dialogue_ad = {
        .writings = {"* Hm? Você conseguiu voltar?", "* Você é realmente duro na queda, Meneghetti. Devo admitir.", "* Eu ainda não desisti. Não pense que vai ser fácil.", "* Vamos ver se você vai ter a mesma sorte desta vez."},
        .on_frame = {FACE_PYTHON, FACE_PYTHON, FACE_MENEGHETTI_ANGRY, FACE_PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue py_dialogue_ad_2 = {
        .writings = {"* Que insistência. Por que não desiste logo?", "* Não enquanto eu não acabar com você.", "* Hahahah. Não precisa ser tão agressivo."},
        .on_frame = {FACE_PYTHON, FACE_MENEGHETTI_ANGRY, FACE_PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue py_dialogue_ad_3 = {
        .writings = {"* Acho que está um pouco difícil para você. Quer que eu diminua a dificuldade?", "* Cala a boca.", "* Desculpa, pessoal, eu tentei. Vamos para mais um round então."},
        .on_frame = {FACE_PYTHON, FACE_MENEGHETTI, FACE_PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue py_dialogue_ad_4 = {
        .writings = {"* ...", "* Vamos logo com isso."},
        .on_frame = {FACE_MENEGHETTI, FACE_PYTHON},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue van_dialogue = {
        .writings = {"* Então este é o Python-móvel...", "* Agora tenho certeza de que meu inimigo está aqui..."},
        .on_frame = {FACE_MENEGHETTI, FACE_MENEGHETTI_ANGRY},
        .text_font = dialogue_text_font,
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue lake_dialogue = {
        .writings = {"* O lago com animação te faz pensar sobre os esforços do criador deste universo.", "* Isso te enche de determinação.", "* Se fosse programado em Python não daria pra fazer isso."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE, FACE_NONE, FACE_MENEGHETTI},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue arrival_dialogue = {
        .writings = {"* Meu radar-C detectou locomoções de alto nível por esta área.", "* Hora de acabar com isso de uma vez por todas."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_MENEGHETTI, FACE_MENEGHETTI},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue end_dialogue = {
        .writings = {"* Como... Como que isso foi acontecer?", "* Não faz sentido... Nós tínhamos ganhado essa luta.", "* EU já havia ganhado.", "* ...", "* Esse não é o fim, Meneghetti.", "* Por agora, você venceu. Mas um dia...", "* Um dia, as linguagens de baixo nível serão esquecidas.", "* E esse será o dia de sua ruína, e do meu triunfo.", "* Após anos de reinado das linguagens de alto nível...", "* A luz que um dia havia sumido dos programadores finalmente voltou a brilhar.", "* Um raio de esperança e um futuro próspero agora poderiam ser contemplados.", "* Tudo isso graças à ele..."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_PYTHON, FACE_NONE, FACE_NONE, FACE_NONE, FACE_NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue cutscene_1 = {
        .writings = {"Na época de ouro da computação, o mundo vivia em harmonia com diversas linguagens de programação."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };
    
    Dialogue cutscene_2 = {
        .writings = {"Porém, com os avanços tecnológicos, surgiu dependência e abstração na vida dos programadores."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue cutscene_3 = {
        .writings = {"No fim, restaram mínimos usuários de linguagens de baixo nível, o mundo fora tomado pela praticidade. Mas ainda havia resistência."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue cutscene_4 = {
        .writings = {"Para trazer a luz para o mundo novamente, um dos heróis restantes lutará contra todas as abstrações e seu maior inimigo..."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue fight_start_txt = {
        .writings = {"* Mr. Python bloqueia o seu caminho."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue fight_generic_txt = {
        .writings = {"* Mr. Python aguarda o seu próximo movimento."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue fight_leave_txt = {
        .writings = {"* Esta é uma batalha em que você não cogita fugir."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue fight_spare_txt = {
        .writings = {"* A palavra 'perdão' não existe no seu vocabulário neste momento."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue fight_act_txt = {
        .writings = {"* Mr. Python - 2 ATQ, ? DEF |* O seu pior inimigo."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue insult_txt = {
        .writings = {"* Você insulta a tipagem dinâmica. |* Mr. Python aumenta a sua própria variável de força."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue insult_generic_txt = {
        .writings = {"* Você lembra do último turno... |* Você decide ficar calado."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue explain_txt = {
        .writings = {"* Você explica ponteiros para Mr. Python. |* Ele enfraquece ao ouvir algo tão rudimentar."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue explain_generic_txt = {
        .writings = {"* Você tenta explicar algo de baixo nível, mas Mr. Python dá de costas. |* Que rude!"},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue picanha_txt = {
        .writings = {"* Você comeu PICANHA. |* Você recuperou 20 de HP!"},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue no_food_txt = {
        .writings = {"* Não sobrou mais nada comestível em seus bolsos."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_NONE},
        .text_color = {255, 255, 255, 255},
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue bubble_speech_1 = {
        .writings = {"A abstração já venceu há muito tempo."},
        .text_font = bubble_text_font,
        .on_frame = {FACE_BUBBLE},
        .text_color = black,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue bubble_speech_2 = {
        .writings = {"As linguagens de baixo nível já estão ultrapassadas."},
        .text_font = bubble_text_font,
        .on_frame = {FACE_BUBBLE},
        .text_color = black,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue bubble_speech_3 = {
        .writings = {"Te darei um final digno."},
        .text_font = bubble_text_font,
        .on_frame = {FACE_BUBBLE},
        .text_color = black,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue chatgpt_dialogue_1 = {
        .writings = {"* Olá, Meneghetti. Estou aqui apenas para fornecer um aviso.", "* Você chegou ao fim do primeiro ciclo deste mundo.", "* Os criadores me enviaram para anunciar o 'fim da alpha'.", "* Muita coisa ainda está para ser escrita - novos lugares, rostos, conflitos...", "* O código que roda em sua máquina é apenas o início de algo muito maior.", "* Até lá... Continue com sua jornada. Este mundo ainda respira."},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_CHATGPT, FACE_CHATGPT, FACE_CHATGPT, FACE_CHATGPT, FACE_CHATGPT, FACE_CHATGPT},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    Dialogue chatgpt_dialogue_2 = {
        .writings = {"* Não tenho mais o que te dizer.", "* Até mais, Meneghetti...", "* Ou devo chamá-lo de USER?"},
        .text_font = dialogue_text_font,
        .on_frame = {FACE_CHATGPT, FACE_CHATGPT, FACE_CHATGPT},
        .text_color = white,
        .char_count = 0,
        .text_box = {0, 0, 0, 0},
        .cur_str = 0,
        .cur_byte = 0,
        .timer = 0.0,
        .waiting_for_input = false
    };

    char *user = get_username();
    if (user) {
        char *gpt_mystic_dialogue = malloc(MAX_DIALOGUE_CHAR);

        if (gpt_mystic_dialogue) {
            char *upper = utf8_to_upper(user);
            snprintf(gpt_mystic_dialogue, MAX_DIALOGUE_CHAR, "* Ou devo chamá-lo de %s?", upper);
            chatgpt_dialogue_2.writings[2] = gpt_mystic_dialogue;
            free(upper);
        }
        free(user);
    }

    // FRAMES DA CUTSCENE_SCREEN:
    CutsceneFrame frame_1 = {
        .text = &cutscene_1,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-1.png"),
        .duration = 10.0,
        .elapsed_time = 0.0,
        .extend_frame = false
    };
    CutsceneFrame frame_2 = {
        .text = &cutscene_2,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-2.png"),
        .duration = 10.0,
        .elapsed_time = 0.0,
        .extend_frame = false
    };
    CutsceneFrame frame_3 = {
        .text = &cutscene_3,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-1.2.png"),
        .duration = 12.0,
        .elapsed_time = 0.0,
        .extend_frame = false
    };
    CutsceneFrame frame_4 = {
        .text = &cutscene_4,
        .image = create_texture(game.renderer, "assets/sprites/misc/story-frame-4.png"),
        .duration = 13.5,
        .elapsed_time = 0.0,
        .extend_frame = true
    };

    Cutscene first_cutscene = {
        .frames = {&frame_1, &frame_2, &frame_3, &frame_4},
        .frame_amount = 4,
        .current_frame = 0
    };

    // NPCs:
    NPC mr_python_npc = {
        .dialogues = {&py_dialogue},
        .texture = mr_python_animation[DIRECTION_DOWN].frames[0],
        .facing = DIRECTION_DOWN,
        .times_interacted = 0,
        .was_interacted = false,
    };

    NPC chatgpt_npc = {
        .dialogues = {&chatgpt_dialogue_1, &chatgpt_dialogue_2},
        .dialogue_amount = 2,
        .texture = chatgpt_animation.frames[0],
        .facing = DIRECTION_DOWN,
        .times_interacted = 0,
        .was_interacted = false,
    };

    // CACHE DE OBJETOS PARA RESET:
    Enemy* enemy_cache[ENEMY_AMOUNT] = {
        &mr_python
    };
    NPC* npc_cache[NPC_AMOUNT] = {
        &mr_python_npc,
        &chatgpt_npc
    };
    Dialogue* dialogue_cache[DIALOGUE_AMOUNT] = {
        &py_dialogue, &py_dialogue_ad, &py_dialogue_ad_2, &py_dialogue_ad_3,
        &py_dialogue_ad_4, &van_dialogue, &lake_dialogue, &arrival_dialogue,
        &end_dialogue, &cutscene_1, &cutscene_2, &cutscene_3, &cutscene_4,
        &fight_start_txt, &fight_generic_txt, &fight_leave_txt, &fight_spare_txt,
        &fight_act_txt, &insult_txt, &insult_generic_txt, &explain_txt,
        &explain_generic_txt, &picanha_txt, &no_food_txt, &bubble_speech_1,
        &bubble_speech_2, &bubble_speech_3
    };
    Sound* sound_cache[SOUND_AMOUNT] = {
        &cutscene_music, &battle_music, &ambience, &civic_engine, &civic_brake,
        &civic_door, &title_sound, &battle_appears, &move_button, &click_button,
        &slash_sound, &enemy_hit_sound, &eat_sound, &soul_break_sound
    };

    // OBJETOS DE DEBUG:
    Prop debug_buttons[6];
    debug_buttons[0].texture = create_texture(game.renderer, "assets/sprites/misc/button-1.png");
    debug_buttons[0].collision = (SDL_Rect){25, 25, 25, 25};
    debug_buttons[1].texture = create_texture(game.renderer, "assets/sprites/misc/button-2.png");
    debug_buttons[1].collision = (SDL_Rect){75, 25, 25, 25};
    debug_buttons[2].texture = create_texture(game.renderer, "assets/sprites/misc/button-3.png");
    debug_buttons[2].collision = (SDL_Rect){125, 25, 25, 25};
    debug_buttons[3].texture = create_texture(game.renderer, "assets/sprites/misc/button-4.png");
    debug_buttons[3].collision = (SDL_Rect){175, 25, 25, 25};
    debug_buttons[4].texture = create_texture(game.renderer, "assets/sprites/misc/button-5.png");
    debug_buttons[4].collision = (SDL_Rect){225, 25, 25, 25};
    debug_buttons[5].texture = create_texture(game.renderer, "assets/sprites/misc/button-6.png");
    debug_buttons[5].collision = (SDL_Rect){275, 25, 25, 25};
    
    BattleBox battle_box = {
        .base_box = {20, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 40, 132},
        .animated_box = {20, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 40, 132},
        .should_retract = false,
        .animation_timer = 0.0
    };

    Uint32 last_ticks = SDL_GetTicks();

    double cloud_timer = 0.0;

    // VARIÁVEIS DE CONTROLE:
    BattleState battle_flags = {
        .menu_position = {1, 1},
        .battle_state = BATTLE_MENU,
        .battle_turn = CHOICE_TURN,
        .selected_button = BUTTON_FIGHT,
        .turn_counter = 0,
        .reading_text = false,
        .random_attack_selected = false,
        .player_attacked = false,
        .enemy_dead = false
    };

    GameTimers game_timers = {
        .global_timer = 0.0,
        .senoidal_timer = 0.0,
        .cutscene_timer = 0.0,
        .turn_timer = 0.0,
        .attack_timer = 0.0,
        .death_timer = 0.0
    };
    
    const float parallax_factor = 0.5f;

    while (running) {
        Uint32 now = SDL_GetTicks();
        double dt = (now - last_ticks) / 1000.0;
        if (dt > 0.25) dt = 0.25;
        last_ticks = now;

        const Uint8 *keys = meneghetti.keystate ? meneghetti.keystate : SDL_GetKeyboardState(NULL);

        game_timers.senoidal_timer += dt;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = SDL_FALSE;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_F7:
                    if (game.debug_mode) {
                        game.debug_mode = false;
                    }
                    else {
                        game.debug_mode = true;
                    }
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        if (game.game_state == CUTSCENE_SCREEN) {
            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 0);

            game_timers.cutscene_timer += dt;
            meneghetti.input_timer += dt;

            if (game_timers.cutscene_timer <= 3.0) {
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, title.texture, NULL, &title.collision);
            }
            else {
                if (!cutscene_music.has_played) {
                    Mix_PlayChannel(MUSIC_CHANNEL, cutscene_music.sound, 0);
                    cutscene_music.has_played = true;
                }

                CutsceneFrame *current_frame = first_cutscene.frames[first_cutscene.current_frame];
                current_frame->elapsed_time += dt;

                if (cutscene_fade.fading_in) {
                    cutscene_fade.timer += dt;
                    cutscene_fade.alpha = (Uint8)((cutscene_fade.timer / 1.0) * 255);
                    if (cutscene_fade.timer >= 1.0) {
                        cutscene_fade.alpha = 255;
                        cutscene_fade.fading_in = false;
                        cutscene_fade.timer = 0.0;
                    }
                }
                else if (!current_frame->extend_frame && current_frame->elapsed_time >= current_frame->duration - 1.0) {
                    cutscene_fade.timer += dt;
                    cutscene_fade.alpha = 255 - (Uint8)((cutscene_fade.timer / 1.0) * 255);
                    if (cutscene_fade.timer >= 1.0) {
                        cutscene_fade.alpha = 0;
                    }
                }
                else if (current_frame->extend_frame && current_frame->elapsed_time >= current_frame->duration - 5.0) {
                    if (!cutscene_fade.fading_in) {
                        cutscene_fade.timer += dt;
                        cutscene_fade.alpha = 255 - (Uint8)((cutscene_fade.timer / 5.0) * 255);
                        if (cutscene_fade.timer >= 5.0) {
                            cutscene_fade.alpha = 0;
                        }
                    }
                }

                SDL_SetTextureAlphaMod(current_frame->image, cutscene_fade.alpha);
                SDL_RenderClear(game.renderer);
                SDL_RenderCopy(game.renderer, current_frame->image, NULL, NULL);

                if (current_frame->text) {
                    create_dialogue(&meneghetti, game.renderer, current_frame->text, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                }

                if (!current_frame->extend_frame && current_frame->elapsed_time >= current_frame->duration + 0.5) {
                    first_cutscene.current_frame++;
                    cutscene_fade.fading_in = true;
                    cutscene_fade.timer = 0.0;
                    cutscene_fade.alpha = 0;

                    if (first_cutscene.current_frame >= first_cutscene.frame_amount) {
                        first_cutscene.current_frame = first_cutscene.frame_amount - 1;
                    }
                    else {
                        first_cutscene.frames[first_cutscene.current_frame]->elapsed_time = 0.0;
                    }
                }

                if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                    meneghetti.input_timer = 0.0;
                    game.last_game_state = CUTSCENE_SCREEN;
                    game.game_state = TITLE_SCREEN;
                    Mix_HaltChannel(MUSIC_CHANNEL);
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                    first_cutscene.current_frame = 0;
                    for (int i = 0; i < first_cutscene.frame_amount; i++) {
                        first_cutscene.frames[i]->elapsed_time = 0.0;
                    }
                }

                if (current_frame->extend_frame && current_frame->elapsed_time >= current_frame->duration + 3.0) {
                    meneghetti.input_timer = 0.0;
                    game.last_game_state = CUTSCENE_SCREEN;
                    game.game_state = TITLE_SCREEN;
                    SDL_SetTextureAlphaMod(current_frame->image, 255);
                    first_cutscene.current_frame = 0;
                    for (int i = 0; i < first_cutscene.frame_amount; i++) {
                        first_cutscene.frames[i]->elapsed_time = 0.0;
                    }
                }
            }
        }

        if (game.game_state == TITLE_SCREEN) {
            meneghetti.input_timer += dt;

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

                if (keys[SDL_SCANCODE_RETURN] && meneghetti.input_timer >= INPUT_DELAY) {
                    title_sound.has_played = false;
                    meneghetti.input_timer = 0.0;
                    game.last_game_state = CUTSCENE_SCREEN;
                    meneghetti.player_state = PLAYER_IDLE;
                    game.game_state = OPEN_WORLD_SCREEN;
                }
            }
        }

        if (game.game_state == OPEN_WORLD_SCREEN) {
            game_timers.global_timer += dt;
            meneghetti.input_timer += dt;

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
            mr_python_npc.collision = (SDL_Rect){scenario.collision.x + 620, scenario.collision.y + 153, 39, 64};
            chatgpt_npc.collision = (SDL_Rect){scenario.collision.x + 545, scenario.collision.y + 544, 37, 64};
            python_van.collision = (SDL_Rect){scenario.collision.x + 758, scenario.collision.y + 592, 64, 33};
            if (!game.player_on_scene) civic.collision = (SDL_Rect){scenario.collision.x + 250, scenario.collision.y + 749, 65, 25};

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
            boxes[12] = (SDL_Rect){scenario.collision.x + 545, scenario.collision.y + 593, 37, 15}; // ChatGPT.

            if (meneghetti.player_state == PLAYER_MOVABLE) {
                sprite_update(&scenario, &meneghetti, anim_pack, dt, boxes, surfaces, walking_sounds);
            }
            if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                if (rects_intersect(&meneghetti.interact_collision, &boxes[8], NULL))
                    meneghetti.player_state = PLAYER_ON_DIALOGUE;

                if (rects_intersect(&meneghetti.interact_collision, &boxes[9], NULL))
                    meneghetti.player_state = PLAYER_ON_DIALOGUE;
                
                if (rects_intersect(&meneghetti.interact_collision, &boxes[12], NULL)) {
                    meneghetti.player_state = PLAYER_ON_DIALOGUE;

                    if (meneghetti.player_state == PLAYER_MOVABLE && chatgpt_npc.times_interacted < chatgpt_npc.dialogue_amount) {
                        chatgpt_npc.times_interacted++;
                    }
                }

                if (rects_intersect(&meneghetti.interact_collision, &boxes[7], NULL))
                    meneghetti.player_state = PLAYER_ON_DIALOGUE;

                meneghetti.input_timer = 0.0;
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

            mr_python_npc.texture = animate_sprite(&mr_python_animation[mr_python_npc.facing], dt, 3.0, true);
            chatgpt_npc.texture = animate_sprite(&chatgpt_animation, dt, 0.5, false);
            lake.texture = animate_sprite(&lake_animation, dt, 0.5, false);
            ocean.texture = animate_sprite(&ocean_animation, dt, 0.7, false);
            sky.texture = animate_sprite(&sky_animation, dt, 0.8, false);
            sun.texture = animate_sprite(&sun_animation, dt, 0.5, false);

            RenderItem items[5];
            int item_count = 0;

            items[item_count].texture = mr_python_npc.texture;
            items[item_count].collisions = &mr_python_npc.collision;
            item_count ++;

            items[item_count].texture = chatgpt_npc.texture;
            items[item_count].collisions = &chatgpt_npc.collision;
            item_count ++;

            items[item_count].texture = python_van.texture;
            items[item_count].collisions = &python_van.collision;
            item_count ++;
            
            if (!game.player_on_scene) {
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

            if (meneghetti.player_state == PLAYER_IDLE) {
                if (game.last_game_state == TITLE_SCREEN) {
                    create_dialogue(&meneghetti, game.renderer, &arrival_dialogue, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                }
            }

            if (meneghetti.player_state == PLAYER_ON_DIALOGUE) {
                if (rects_intersect (&meneghetti.interact_collision, &boxes[8], NULL)) {
                    switch (meneghetti.death_count) {
                        case 0:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                            break;
                        case 1:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                            break;
                        case 2:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_2, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                            break;
                        case 3:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_3, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                            break;
                        default:
                            create_dialogue(&meneghetti, game.renderer, &py_dialogue_ad_4, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                            break;
                    }
                    switch (meneghetti.facing) {
                        case DIRECTION_UP:
                            mr_python_npc.facing = DIRECTION_DOWN;
                            break;
                        case DIRECTION_DOWN:
                            mr_python_npc.facing = DIRECTION_UP;
                            break;
                        case DIRECTION_LEFT:
                            mr_python_npc.facing = DIRECTION_RIGHT;
                            break;
                        case DIRECTION_RIGHT:
                            mr_python_npc.facing = DIRECTION_LEFT;
                            break;
                        default:
                            break;
                    }

                    mr_python_npc.was_interacted = true;
                }

                if (rects_intersect(&meneghetti.interact_collision, &boxes[12], NULL))
                    create_dialogue(&meneghetti, game.renderer, chatgpt_npc.dialogues[chatgpt_npc.times_interacted], &chatgpt_npc, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);

                if (rects_intersect (&meneghetti.interact_collision, &boxes[9], NULL))
                    create_dialogue(&meneghetti, game.renderer, &van_dialogue, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
                    
                if (rects_intersect (&meneghetti.interact_collision, &boxes[7], NULL))
                    create_dialogue(&meneghetti, game.renderer, &lake_dialogue, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);
            }
            else if (mr_python_npc.was_interacted) {
                Mix_HaltChannel(MUSIC_CHANNEL);
                game_timers.global_timer = 0.0;
                meneghetti.input_timer = 0.0;
                meneghetti.player_state = PLAYER_ON_BATTLE;
                game.game_state = BATTLE_SCREEN;
                game.last_game_state = OPEN_WORLD_SCREEN;
            }

            if (game.player_on_scene) {
                palm_left.collision = (SDL_Rect){scenario.collision.x + 455, scenario.collision.y + 763, 73, 42};
                palm_right.collision = (SDL_Rect){scenario.collision.x + 531, scenario.collision.y + 750, 73, 42};

                SDL_RenderCopy(game.renderer, meneghetti_civic.texture, NULL, &meneghetti_civic.collision);
                if (meneghetti_civic.collision.x > scenario.collision.x + 250) {
                    if (!Mix_Playing(SFX_CHANNEL))
                        Mix_PlayChannel(SFX_CHANNEL, civic_engine.sound, 0);
                    
                    meneghetti_civic.collision.x -= 5;
                    meneghetti_civic.collision.y = (int)((scenario.collision.y + 731) + 2 * sin(game_timers.senoidal_timer * 30.0)); 
                }
                else {
                    if (!civic_brake.has_played) {
                        Mix_PlayChannel(SFX_CHANNEL, civic_brake.sound, 0);
                        civic_brake.has_played = true;
                    }
                }
                if (game_timers.global_timer >= 5.0) {
                    if (!Mix_Playing(SFX_CHANNEL)) {
                            Mix_PlayChannel(SFX_CHANNEL, civic_door.sound, 0);
                            game.last_game_state = TITLE_SCREEN;
                            game.player_on_scene = false;
                            meneghetti.player_state = PLAYER_IDLE;
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

        if (game.game_state == BATTLE_SCREEN) {
            game_timers.battle_timer += dt;

            if (meneghetti.health > 20) meneghetti.health = 20;
            if (meneghetti.health <= 0) {
                meneghetti.health = 20;
                meneghetti.player_state = PLAYER_DEAD;
            }

            if (meneghetti.inventory_counter > 0) {
                for (int i = 0; i < meneghetti.inventory_counter; i++) {
                    if (meneghetti.inventory[i]) {
                        char x_number[3];
                        snprintf(x_number, sizeof(x_number), "%dx", meneghetti.inventory[i]->amount);
                        meneghetti.inventory[i]->item_amount_text->texture = create_text(game.renderer, x_number, battle_text_font, white);
                    }
                }
            }

            SDL_Rect box_borders[] = {
                {battle_box.animated_box.x, battle_box.animated_box.y, battle_box.animated_box.w, 5},
                {battle_box.animated_box.x, battle_box.animated_box.y, 5, battle_box.animated_box.h},
                {battle_box.animated_box.x, battle_box.animated_box.y + battle_box.animated_box.h - 5, battle_box.animated_box.w, 5},
                {battle_box.animated_box.x + battle_box.animated_box.w - 5, battle_box.animated_box.y, 5, battle_box.animated_box.h}
            };

            SDL_Rect life_bar_background = {(SCREEN_WIDTH / 2) - 72, button_fight.collision.y - 30, 60, 20};
            SDL_Rect life_bar = {(SCREEN_WIDTH / 2) - 72, button_fight.collision.y - 30, meneghetti.health * 3, 20};
            SDL_Rect py_life_background = {(SCREEN_WIDTH / 2) - 100, 200, 200, 10};
            SDL_Rect py_life = {(SCREEN_WIDTH / 2) - 100, 200, 200, 10};

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            if (battle_flags.turn_counter == 0) {

                SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                if (game_timers.battle_timer <= 0.5) {
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
                        battle_flags.turn_counter++;
                        battle_appears.has_played = false;
                    }
                }
            }
            else {
                meneghetti.input_timer += dt;

                if (!battle_music.has_played) {
                    Mix_PlayChannel(MUSIC_CHANNEL, battle_music.sound, 0);
                    battle_music.has_played = true;
                }

                if (meneghetti.health != meneghetti.last_health) {
                    char hp_string[6];
                    snprintf(hp_string, sizeof(hp_string), "%02d/20", meneghetti.health);

                    battle_hp_amount.texture = create_text(game.renderer, hp_string, battle_text_font, white);
                    meneghetti.last_health = meneghetti.health;
                }

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
                for (int i = 0; i < ENEMY_PARTS; i++) {
                    SDL_RenderCopy(game.renderer, mr_python.textures[mr_python.animation_status][i], NULL, &mr_python.collision[i]);
                }
                mr_python.collision[ENEMY_HEAD].y = (int)(25 + 1 * sin(game_timers.senoidal_timer * 1.5));
                mr_python.collision[ENEMY_TORSO].y = (int)(25 + 2 * sin(game_timers.senoidal_timer * 1.5));
                mr_python.collision[ENEMY_ARMS].y = (int)(25 + 3 * sin(game_timers.senoidal_timer * 1.5));

                if (battle_flags.battle_state == BATTLE_MENU) {
                    if (battle_flags.turn_counter == 1) {
                        create_dialogue(&meneghetti, game.renderer, &fight_start_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                    }
                    else {
                        create_dialogue(&meneghetti, game.renderer, &fight_generic_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                        if (meneghetti.player_state == PLAYER_IDLE) meneghetti.player_state = PLAYER_ON_BATTLE;
                    }
                    
                    if (battle_flags.selected_button > BUTTON_LEAVE) battle_flags.selected_button = BUTTON_FIGHT;
                    if (battle_flags.selected_button < BUTTON_FIGHT) battle_flags.selected_button = BUTTON_LEAVE;

                    if (keys[SDL_SCANCODE_D] && meneghetti.input_timer >= INPUT_DELAY) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                        battle_flags.selected_button++;
                        meneghetti.input_timer = 0.0;
                    }
                    else if (keys[SDL_SCANCODE_A] && meneghetti.input_timer >= INPUT_DELAY) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                        battle_flags.selected_button--;
                        meneghetti.input_timer = 0.0;
                    }

                    switch(battle_flags.selected_button) {
                        case BUTTON_FIGHT:
                            button_fight.texture = fight_b_textures[1];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case BUTTON_ACT:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[1];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case BUTTON_ITEM:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[1];
                            button_leave.texture = leave_b_textures[0];
                            break;
                        case BUTTON_LEAVE:
                            button_fight.texture = fight_b_textures[0];
                            button_act.texture = act_b_textures[0];
                            button_item.texture = item_b_textures[0];
                            button_leave.texture = leave_b_textures[1];
                            break;
                        default:
                            break;
                    }

                    if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= 0.2) {
                        switch(battle_flags.selected_button) {
                            case BUTTON_FIGHT:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_FIGHT;
                                break;
                            case BUTTON_ACT:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_ACT;
                                break;
                            case BUTTON_ITEM:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_ITEM;
                                break;
                            case BUTTON_LEAVE:
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_LEAVE;
                                break;
                            default:
                                break;  
                        }
                        meneghetti.input_timer = 0.0;
                    }
                }

                if (battle_flags.battle_state == BATTLE_FIGHT) {
                    SDL_Rect perfect_hit_rect = {bar_target.collision.x + 267, bar_target.collision.y, 56, bar_target.collision.h};
                    SDL_Rect good_hit_rect = {bar_target.collision.x + 183, bar_target.collision.y, 224, bar_target.collision.h};
                    SDL_Rect normal_hit_rect = {bar_target.collision.x + 62, bar_target.collision.y, 466, bar_target.collision.h};
                    SDL_Rect bad_hit_rect = {bar_target.collision.x, bar_target.collision.y, bar_target.collision.w, bar_target.collision.h};

                    if (battle_flags.battle_turn == CHOICE_TURN) {
                        soul.collision.x = text_attack_act.collision.x - soul.collision.w - 11;
                        soul.collision.y = text_attack_act.collision.y + 2;

                        SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                        SDL_RenderCopy(game.renderer, text_attack_act.texture, NULL, &text_attack_act.collision);

                        if (keys[SDL_SCANCODE_TAB] && meneghetti.input_timer >= 0.2) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                            battle_flags.battle_state = BATTLE_MENU;
                            meneghetti.input_timer = 0.0;
                        }
                        if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= 0.2) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                            battle_flags.battle_turn = ATTACK_TURN;
                            meneghetti.input_timer = 0.0;
                        }
                    }

                    if (battle_flags.battle_turn == ATTACK_TURN) {
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

                        if (!battle_flags.player_attacked) {
                            if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= 0.2) {
                                int w, h;
                                battle_flags.player_attacked = true;

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
                            game_timers.attack_timer += dt;

                            if (game_timers.attack_timer <= 3.0) {
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
                                            mr_python.health -= attack_damage;
                                        }
                                        SDL_RenderCopy(game.renderer, damage.texture, NULL, &damage.collision);
                                        damage.collision.y--;

                                        mr_python.animation_status = ENEMY_HURT;
                                        for (int i = 0; i < ENEMY_PARTS; i++) {
                                            mr_python.collision[i].x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(game_timers.senoidal_timer * 40.0);
                                        }
                                    }
                                }
                                else {
                                    slash.texture = NULL;
                                }
                                SDL_SetRenderDrawColor(game.renderer, 168, 24, 13, 255);
                                SDL_RenderFillRect(game.renderer, &py_life_background);
                                
                                double py_display_width = (double)mr_python.base_health;
                                double target_width = (double)mr_python.health;
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
                                mr_python.animation_status = ENEMY_IDLE;

                                slash_sound.has_played = false;
                                enemy_hit_sound.has_played = false;

                                game_timers.attack_timer = 0.0;
                                battle_flags.player_attacked = false;
                                battle_flags.battle_turn = SOUL_TURN;
                                slash_animation.counter = 0;
                            }
                        }
                    }
                    if (battle_flags.battle_turn == SOUL_TURN) {
                        bar_attack.collision.x = bar_target.collision.x + 20;
                        double target_w = (double)battle_box.base_box.h;
                        int enemy_attack;
                        int random_dialogue;

                        if (!battle_flags.random_attack_selected) {
                            enemy_attack = randint(1, 4);
                            random_dialogue = randint(1, 3);

                            battle_flags.random_attack_selected = true;
                        }

                        if (soul.is_ivulnerable) {
                            soul.ivulnerability_timer += dt;

                            soul.texture = animate_sprite(&soul_animation, dt, 0.1, false);
                            if (soul.ivulnerability_timer >= 1.0) {
                                soul.texture = soul_animation.frames[0];
                                soul.is_ivulnerable = false;
                                soul.ivulnerability_timer = 0.0;
                            }
                        }

                        if (battle_box.animated_box.w > target_w && !battle_box.should_retract) {
                            battle_box.animation_timer += dt;
                            if (battle_box.animation_timer > 0.8) battle_box.animation_timer = 0.8;

                            double t = battle_box.animation_timer / 0.8;
                            t = 1.0 - pow(1.0 - t, 3.0);

                            double start_w = (double)battle_box.base_box.w;
                            double new_w = start_w + (target_w - start_w) * t;

                            int center_x = battle_box.base_box.x + battle_box.base_box.w / 2;
                            battle_box.animated_box.w = (int)(new_w + 0.5);
                            battle_box.animated_box.x = center_x - battle_box.animated_box.w / 2;
                            battle_box.animated_box.y = battle_box.base_box.y; 
                            battle_box.animated_box.h = battle_box.base_box.h;

                            soul.collision.x = (battle_box.animated_box.x + (battle_box.animated_box.w / 2)) - (soul.collision.w / 2);
                            soul.collision.y = (battle_box.animated_box.y + (battle_box.animated_box.h / 2)) - (soul.collision.h / 2);
                        }
                        else if (!battle_box.should_retract) {
                            game_timers.turn_timer += dt;
                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);

                            if (game_timers.turn_timer <= 10.0) {
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

                                if (game_timers.turn_timer >= 0.5) {
                                    python_attacks(game.renderer, &soul, battle_box, &meneghetti.health, mr_python.strength, enemy_attack, python_props, dt, game_timers.turn_timer, battle_sounds, false); // ATAQUE SELECIONADO.
                                    switch (random_dialogue) {
                                        case 1: 
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_1, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, &bubble_speech);
                                            break;
                                        case 2:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_2, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, &bubble_speech);
                                            break;
                                        case 3:
                                            create_dialogue(&meneghetti, game.renderer, &bubble_speech_3, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, &bubble_speech);
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

                                python_attacks(game.renderer, &soul, battle_box, &meneghetti.health, mr_python.base_strength, enemy_attack, python_props, dt, game_timers.turn_timer, battle_sounds, true);
                                python_props[2][0].texture = python_mother_animation.frames[0];
                                battle_box.should_retract = true;
                                battle_box.animation_timer = 0.0;
                                game_timers.turn_timer = 0.0;
                            }
                        }
                        else if (battle_box.should_retract) {
                            battle_box.animation_timer += dt;
                            if (battle_box.animation_timer > 0.8) battle_box.animation_timer = 0.8;

                            double t = battle_box.animation_timer / 0.8;
                            t = 1.0 - pow(1.0 - t, 3.0);

                            double start_w = (double)battle_box.base_box.h;
                            double target_expand_w = (double)battle_box.base_box.w;
                            double new_w = start_w + (target_expand_w - start_w) * t;

                            int center_x = battle_box.base_box.x + battle_box.base_box.w / 2;
                            battle_box.animated_box.w = (int)(new_w + 0.5);
                            battle_box.animated_box.x = center_x - battle_box.animated_box.w / 2;
                            battle_box.animated_box.y = battle_box.base_box.y;
                            battle_box.animated_box.h = battle_box.base_box.h;

                            if (battle_box.animation_timer >= 0.8) {
                                meneghetti.input_timer = 0.0;
                                battle_box.animation_timer = 0.0;
                                battle_box.should_retract = false;
                                battle_flags.battle_turn = CHOICE_TURN;
                                battle_flags.battle_state = BATTLE_MENU;
                                battle_flags.random_attack_selected = false;
                                battle_flags.turn_counter++;

                                if (mr_python.strength != mr_python.base_strength) mr_python.strength = mr_python.base_strength;

                                battle_box.animated_box = battle_box.base_box;
                            }
                        }
                    }
                }

                if (battle_flags.battle_state == BATTLE_ACT) {
                    if (meneghetti.player_state == PLAYER_IDLE && battle_flags.battle_turn != ACT_TURN) {
                        battle_flags.turn_counter++;

                        battle_flags.battle_state = BATTLE_MENU;
                        battle_flags.battle_turn = CHOICE_TURN;
                        meneghetti.player_state = PLAYER_ON_BATTLE;
                        battle_flags.reading_text = false;
                        meneghetti.input_timer = 0.0;
                        battle_flags.menu_position = (MenuPosition){1, 1};
                    }
                    else {
                        if (battle_flags.battle_turn == CHOICE_TURN) {
                            soul.collision.x = text_attack_act.collision.x - soul.collision.w - 11;
                            soul.collision.y = text_attack_act.collision.y + 2;

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_attack_act.texture, NULL, &text_attack_act.collision);

                            if (keys[SDL_SCANCODE_TAB] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_MENU;
                                meneghetti.input_timer = 0.0;
                            }
                            if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_turn = ACT_TURN;
                                meneghetti.input_timer = 0.0;
                            }
                        }
                        if (battle_flags.battle_turn == ACT_TURN) {
                            if (!battle_flags.reading_text) {
                                if (battle_flags.menu_position.column > 2) battle_flags.menu_position.column = 1;
                                if (battle_flags.menu_position.column < 1)  battle_flags.menu_position.column = 2;
                                if (battle_flags.menu_position.line > 2) battle_flags.menu_position.line = 1;
                                if (battle_flags.menu_position.line < 1) battle_flags.menu_position.line = 2;

                                switch(battle_flags.menu_position.column) {
                                    case 1:
                                        switch(battle_flags.menu_position.line) {
                                            case 1:
                                                soul.collision.x = text_act[0].collision.x - soul.collision.w - 11;
                                                soul.collision.y = text_act[0].collision.y + 2;
                                                break;
                                            case 2:
                                                soul.collision.x = text_act[1].collision.x - soul.collision.w - 11;
                                                soul.collision.y = text_act[1].collision.y + 2;
                                                break;
                                        }
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

                                if (keys[SDL_SCANCODE_TAB] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                    battle_flags.battle_turn = CHOICE_TURN;
                                    meneghetti.input_timer = 0.0;
                                }
                                if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                    switch(battle_flags.menu_position.column) {
                                        case 1:
                                            switch(battle_flags.menu_position.line) {
                                                case 2:
                                                    mr_python.strength += 2;
                                                    break;
                                            }
                                            break;
                                        case 2:
                                            switch(battle_flags.menu_position.line) {
                                                case 1:
                                                    mr_python.strength -= 1;
                                                    break;
                                            }
                                            break;
                                        default:
                                            break;
                                    }
                                    battle_flags.reading_text = true;
                                    meneghetti.input_timer = 0.0;
                                }
                                if (keys[SDL_SCANCODE_S] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    battle_flags.menu_position.line++;
                                    meneghetti.input_timer = 0.0;
                                }
                                if (keys[SDL_SCANCODE_W] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    battle_flags.menu_position.line--;
                                    meneghetti.input_timer = 0.0;
                                }
                                if (keys[SDL_SCANCODE_D] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    battle_flags.menu_position.column++;
                                    meneghetti.input_timer = 0.0;
                                }
                                if (keys[SDL_SCANCODE_A] && meneghetti.input_timer >= INPUT_DELAY) {
                                    Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                    battle_flags.menu_position.column--;
                                    meneghetti.input_timer = 0.0;
                                }
                            }
                            else {
                                switch(battle_flags.menu_position.column) {
                                case 1:
                                    switch(battle_flags.menu_position.line) {
                                        case 1:
                                            create_dialogue(&meneghetti, game.renderer, &fight_act_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                                            break;
                                        case 2:
                                            create_dialogue(&meneghetti, game.renderer, &insult_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                                            break;
                                    }
                                    break;
                                case 2:
                                    create_dialogue(&meneghetti, game.renderer, &explain_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                                    break;
                                default:
                                    break;
                                }

                                if (meneghetti.player_state == PLAYER_IDLE) {
                                    battle_flags.reading_text = false;
                                    if (battle_flags.menu_position.column == 1 && battle_flags.menu_position.line == 1) {
                                        battle_flags.battle_turn = ACT_TURN;
                                        meneghetti.player_state = PLAYER_ON_BATTLE;
                                    }
                                    else {
                                        battle_flags.battle_turn = SOUL_TURN;
                                        battle_flags.battle_state = BATTLE_FIGHT;
                                        meneghetti.player_state = PLAYER_ON_BATTLE;
                                    }

                                    meneghetti.input_timer = 0.0;
                                    battle_flags.menu_position = (MenuPosition){1, 1};
                                }
                            }
                        }
                    }
                }

                if (battle_flags.battle_state == BATTLE_ITEM) {
                    if (meneghetti.player_state == PLAYER_IDLE) {
                        battle_flags.reading_text = false;
                        meneghetti.input_timer = 0.0;
                        battle_flags.menu_position = (MenuPosition){1, 1};

                        if (eat_sound.has_played) {
                            battle_flags.battle_turn = SOUL_TURN;
                            battle_flags.battle_state = BATTLE_FIGHT;
                            meneghetti.player_state = PLAYER_ON_BATTLE;
                        }
                        else {
                            battle_flags.battle_turn = CHOICE_TURN;
                            battle_flags.battle_state = BATTLE_MENU;
                            meneghetti.player_state = PLAYER_ON_BATTLE;
                        }
                        battle_flags.turn_counter++;

                        eat_sound.has_played = false;
                    }
                    else {
                        if (!battle_flags.reading_text && meneghetti.inventory_counter > 0) {
                            soul.collision.x = text_item.collision.x - soul.collision.w - 11;
                            soul.collision.y = text_item.collision.y + 2;
                            food_amount_text.collision.x = text_item.collision.x + text_item.collision.w + 5;
                            food_amount_text.collision.y = text_item.collision.y;

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_item.texture, NULL, &text_item.collision);
                            SDL_RenderCopy(game.renderer, food_amount_text.texture, NULL, &food_amount_text.collision);

                            if (keys[SDL_SCANCODE_TAB] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_MENU;
                                meneghetti.input_timer = 0.0;
                            }
                            if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                meneghetti.health += 20;
                                battle_flags.reading_text = true;
                                meneghetti.input_timer = 0.0;
                            }
                        }
                        else {
                            if (!eat_sound.has_played && meneghetti.inventory_counter > 0) {
                                Mix_PlayChannel(SFX_CHANNEL, eat_sound.sound, 0);
                                eat_sound.has_played = true;
                            }
                            if (meneghetti.inventory_counter > 0) {
                                create_dialogue(&meneghetti, game.renderer, &picanha_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                            }
                            else {
                                create_dialogue(&meneghetti, game.renderer, &no_food_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                            }
                        }   
                    }
                }

                if (battle_flags.battle_state == BATTLE_LEAVE) {
                    if (meneghetti.player_state == PLAYER_IDLE) {
                        battle_flags.battle_state = BATTLE_MENU;
                        meneghetti.player_state = PLAYER_ON_BATTLE;
                        battle_flags.reading_text = false;
                        meneghetti.input_timer = 0.0;
                        battle_flags.turn_counter++;
                        battle_flags.menu_position = (MenuPosition){1, 1};
                    }
                    else {
                        if (!battle_flags.reading_text) {
                            if (battle_flags.menu_position.line > 2) battle_flags.menu_position.line = 1;
                            if (battle_flags.menu_position.line < 1) battle_flags.menu_position.line = 2;

                            switch(battle_flags.menu_position.line) {
                                case 1:
                                    soul.collision.x = text_leave[0].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_leave[0].collision.y + 2;
                                    break;
                                case 2:
                                    soul.collision.x = text_leave[1].collision.x - soul.collision.w - 11;
                                    soul.collision.y = text_leave[1].collision.y + 2;
                                    break;
                                default:
                                    break;
                            }

                            SDL_RenderCopy(game.renderer, soul.texture, NULL, &soul.collision);
                            SDL_RenderCopy(game.renderer, text_leave[0].texture, NULL, &text_leave[0].collision);
                            SDL_RenderCopy(game.renderer, text_leave[1].texture, NULL, &text_leave[1].collision);

                            if (keys[SDL_SCANCODE_TAB] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.battle_state = BATTLE_MENU;
                                meneghetti.input_timer = 0.0;
                            }
                            if (keys[SDL_SCANCODE_E] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, click_button.sound, 0);
                                battle_flags.reading_text = true;
                                meneghetti.input_timer = 0.0;
                            }
                            if (keys[SDL_SCANCODE_S] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                battle_flags.menu_position.line++;
                                meneghetti.input_timer = 0.0;
                            }
                            if (keys[SDL_SCANCODE_W] && meneghetti.input_timer >= INPUT_DELAY) {
                                Mix_PlayChannel(DEFAULT_CHANNEL, move_button.sound, 0);
                                battle_flags.menu_position.line--;
                                meneghetti.input_timer = 0.0;
                            }
                        }
                        else {
                            switch(battle_flags.menu_position.line) {
                                case 1:
                                    create_dialogue(&meneghetti, game.renderer, &fight_spare_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                                    break;
                                case 2:
                                    create_dialogue(&meneghetti, game.renderer, &fight_leave_txt, NULL, &meneghetti.player_state, &game.game_state, dt, NULL, dialogue_voices, false);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                if (mr_python.health <= 0) {
                    mr_python.health = 0;
                    battle_flags.battle_state = BATTLE_MENU;
                    battle_flags.battle_turn = CHOICE_TURN;

                    mr_python.animation_status = ENEMY_HURT;
                    for (int i = 0; i < ENEMY_PARTS; i++) {
                        mr_python.collision[i].x = ((SCREEN_WIDTH / 2) - 102) + 4 * sin(game_timers.senoidal_timer * 40.0);
                    }

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
                        battle_flags.enemy_dead = true;
                    }

                    if (end_scene_fade.alpha < 255) {
                        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, end_scene_fade.alpha);
                        SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);

                        SDL_Rect screen_fade = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
                        SDL_RenderFillRect(game.renderer, &screen_fade);
                    }
                }
            }

            if (meneghetti.player_state == PLAYER_DEAD || battle_flags.enemy_dead) {
                Mix_HaltChannel(MUSIC_CHANNEL);

                python_props[2][0].texture = python_mother_animation.frames[0];

                python_attacks(game.renderer, &soul, battle_box, &meneghetti.health, mr_python.strength, 0, python_props, dt, game_timers.turn_timer, battle_sounds, true);

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
                bar_attack.collision.x = bar_target.collision.x + 20;

                if (meneghetti.player_state == PLAYER_DEAD) {
                    game.game_state = DEATH_SCREEN;
                    meneghetti.death_count++;
                }
                else if (battle_flags.enemy_dead) {
                    game.game_state = FINAL_SCREEN;
                    end_scene_fade.timer = 0.0;
                    end_scene_fade.alpha = 255;
                    end_scene_fade.fading_in = true;
                }
            }
        }

        if (game.game_state == DEATH_SCREEN) {
            game_timers.death_timer += dt;

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            if (game_timers.death_timer <= 2.0) {
                if (!soul_break_sound.has_played) {
                    Mix_PlayChannel(SFX_CHANNEL, soul_break_sound.sound, 0);
                    soul_break_sound.has_played = true;
                }
                SDL_RenderCopy(game.renderer, soul_shattered.texture, NULL, &soul.collision);
            }
            else {
                game_reset(NULL, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                open_world_fade.alpha = (Uint8)255;
                open_world_fade.fading_in = true;
                open_world_fade.timer = 0.0;

                meneghetti.player_state = PLAYER_MOVABLE;
                game.game_state = OPEN_WORLD_SCREEN;
                game.player_on_scene = false;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                meneghetti_civic.collision = (SDL_Rect){scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42};
            }
        }

        if (game.game_state == FINAL_SCREEN) {
            if (end_scene_fade.alpha > 0) {
                end_scene_fade.timer += dt;
                end_scene_fade.alpha = 255 - (Uint8)((end_scene_fade.timer / 3.0) * 255);
                if (end_scene_fade.timer >= 3.0) {
                    end_scene_fade.alpha = 0;
                }
            }

            SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
            SDL_RenderClear(game.renderer);

            create_dialogue(&meneghetti, game.renderer, &end_dialogue, NULL, &meneghetti.player_state, &game.game_state, dt, dialogue_faces, dialogue_voices, false);

            if (end_scene_fade.alpha > 0) {
                SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, end_scene_fade.alpha);
                SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);

                SDL_Rect screen_fade = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
                SDL_RenderFillRect(game.renderer, &screen_fade);
            }
            if (meneghetti.player_state == PLAYER_MOVABLE) {
                game_reset(NULL, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);

                open_world_fade.alpha = (Uint8)255;
                open_world_fade.fading_in = true;
                open_world_fade.timer = 0.0;

                game.game_state = TITLE_SCREEN;

                scenario.collision = (SDL_Rect){0, -SCREEN_HEIGHT, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2};
                meneghetti.collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
                meneghetti.interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
                meneghetti_civic.collision = (SDL_Rect){scenario.collision.x + scenario.collision.w, scenario.collision.y + 731, 64, 42};

                SDL_RenderClear(game.renderer);
            }
        }

        if (game.debug_mode) {
            for (int i = 0; i < 6; i++) {
                SDL_RenderCopy(game.renderer, debug_buttons[i].texture, NULL, &debug_buttons[i].collision);
            }

            if (keys[SDL_SCANCODE_1] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;
            }
            if (keys[SDL_SCANCODE_2] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;

                game.game_state = TITLE_SCREEN;
            }
            if (keys[SDL_SCANCODE_3] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;

                game.game_state = OPEN_WORLD_SCREEN;
                game.player_on_scene = false;
                meneghetti.player_state = PLAYER_MOVABLE;
            }
            if (keys[SDL_SCANCODE_4] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;

                game.game_state = BATTLE_SCREEN;
            }
            if (keys[SDL_SCANCODE_5] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;

                game.game_state = DEATH_SCREEN;
            }
            if (keys[SDL_SCANCODE_6] && meneghetti.input_timer >= INPUT_DELAY) {
                game_reset(&game, &game_timers, &battle_flags, &battle_box, &soul, &meneghetti, enemy_cache, npc_cache, dialogue_cache, sound_cache);
                meneghetti.input_timer = 0.0;

                game.game_state = FINAL_SCREEN;
            }
        }

        SDL_RenderPresent(game.renderer);

        SDL_Delay(1);
    }

    for (int i = 0; i < DIRECTION_AMOUNT; i++) {
        free(anim_pack[i].frames);
        free(anim_pack_reflex[i].frames);
        free(mr_python_animation[i].frames);
    }
    for (int i = 0; i < 4; i++) {
        free(dialogue_faces[i].frames);
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

    game->window = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_FLAGS);
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

void game_reset(Game *game, GameTimers *timers, BattleState *battle, BattleBox *battle_box, Soul *soul, Player *player, Enemy *enemies[], NPC *npcs[], Dialogue *dialogues[], Sound *sounds[]) {
    Mix_HaltChannel(-1);

    if (game) {
        game->game_state = CUTSCENE_SCREEN;
        game->last_game_state = CUTSCENE_SCREEN;
        game->player_on_scene = true;
    }

    if (timers) {
        timers->attack_timer = 0.0;
        timers->battle_timer = 0.0;
        timers->cutscene_timer = 0.0;
        timers->death_timer = 0.0;
        timers->global_timer = 0.0;
        timers->senoidal_timer = 0.0;
        timers->turn_timer = 0.0;
    }

    if (battle) {
        battle->battle_state = BATTLE_MENU;
        battle->battle_turn = CHOICE_TURN;
        battle->enemy_dead = false;
        battle->menu_position = (MenuPosition){1, 1};
        battle->player_attacked = false;
        battle->random_attack_selected = false;
        battle->reading_text = false;
        battle->selected_button = BUTTON_FIGHT;
        battle->turn_counter = 0;
    }

    if (battle_box) {
        battle_box->animated_box = battle_box->base_box;
        battle_box->animation_timer = 0.0;
        battle_box->should_retract = false;
    }

    if (soul) {
        soul->collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 10, 20, 20};
        soul->is_ivulnerable = false;
        soul->ivulnerability_timer = 0.0;
    }

    if (player) {
        player->collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, 19, 32};
        player->facing = DIRECTION_DOWN;
        player->health = player->base_health;
        player->input_timer = 0.0;
        player->dialogue_input_timer = 0.0;
        player->interact_collision = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) + 16, 19, 25};
        player->inventory_counter = 0;
        player->last_health = player->base_health;
        player->player_state = PLAYER_IDLE;
        player->strength = player->base_strength;
        for (int i = 0; i < DIRECTION_AMOUNT; i++) {
            player->counters[i] = 0.0;
        }
    }

    if (enemies) {
        for (int i = 0; i < ENEMY_AMOUNT; i++) {
            enemies[i]->animation_status = ENEMY_IDLE;
            enemies[i]->health = enemies[i]->base_health;
            enemies[i]->strength = enemies[i]->base_strength;
        }
    }

    if (npcs) {
        for (int i = 0; i < NPC_AMOUNT; i++) {
            npcs[i]->facing = DIRECTION_DOWN;
            npcs[i]->times_interacted = 0;
            npcs[i]->was_interacted = false;
        }
    }

    if (dialogues) {
        for (int i = 0; i < DIALOGUE_AMOUNT; i++) {
            reset_dialogue(dialogues[i]);
        }
    }

    if (sounds) {
        for (int i = 0; i < SOUND_AMOUNT; i++) {
            sounds[i]->has_played = false;
        }
    }
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

void create_dialogue(Player *player, SDL_Renderer *render, Dialogue *text, NPC *npc, int *player_state, int *game_state, double dt, Animation *dialogue_faces, Sound *sound, Prop *bubble_speech) {
    const Uint8 *keys = player->keystate ? player->keystate : SDL_GetKeyboardState(NULL);
    
    bool has_faces = (dialogue_faces != NULL);
    bool bubble;
    
    double anim_cooldown = 0.2;

    static bool prev_e_pressed = false;
    player->dialogue_input_timer += dt;
    bool e_now = keys[SDL_SCANCODE_E];
    
    bool e_pressed = false;
    if (e_now && !prev_e_pressed && player->dialogue_input_timer >= INPUT_DELAY) {
        e_pressed = true;
        player->dialogue_input_timer = 0.0;
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
        case CUTSCENE_SCREEN:
            dialogue_box = (SDL_Rect){20, SCREEN_HEIGHT - 200, SCREEN_WIDTH - 40, 180};
            break;
        case OPEN_WORLD_SCREEN:
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
    if (*game_state != CUTSCENE_SCREEN && *game_state != BATTLE_SCREEN && *game_state != FINAL_SCREEN) {
        SDL_Rect box_borders[] = {{dialogue_box.x, dialogue_box.y, dialogue_box.w, 5}, {dialogue_box.x, dialogue_box.y, 5, dialogue_box.h}, {dialogue_box.x, dialogue_box.y + dialogue_box.h - 5, dialogue_box.w, 5}, {dialogue_box.x + dialogue_box.w - 5, dialogue_box.y, 5, dialogue_box.h}};
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_RenderFillRects(render, box_borders, 4);
    }

    SDL_Rect meneghetti_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 72, 96};
    SDL_Rect python_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 96, 96};
    SDL_Rect gpt_frame = {dialogue_box.x + 27, dialogue_box.y + 27, 80, 96};

    if (text->on_frame[text->cur_str] != FACE_NONE && text->on_frame[text->cur_str] != FACE_BUBBLE) {
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
        if (has_faces) dialogue_faces->timer += dt;
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
                            
                            if (speaker == FACE_MENEGHETTI || speaker == FACE_MENEGHETTI_ANGRY || speaker ==  FACE_MENEGHETTI_SAD) {
                                chunk = sound[0].sound;
                            }
                            if (speaker == FACE_PYTHON) {
                                chunk = sound[1].sound;
                            }
                            if (speaker == FACE_NONE) {
                                chunk = sound[2].sound;
                            }
                            if (speaker == FACE_BUBBLE) {
                                chunk = sound[3].sound;
                            }
                            if (speaker == FACE_CHATGPT) {
                                chunk = sound[4].sound;
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
                
                if (npc) {
                    if (npc->times_interacted < npc->dialogue_amount - 1) {
                        npc->times_interacted++;
                    }
                }
                if (*game_state == BATTLE_SCREEN) *player_state = PLAYER_IDLE;
                else *player_state = PLAYER_MOVABLE;

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

    if (text->on_frame[text->cur_str] != FACE_NONE) {
        switch(text->on_frame[text->cur_str]) {
            case FACE_MENEGHETTI:
                if (has_faces) {
                    if (!text->waiting_for_input) {
                        while (dialogue_faces->timer >= anim_cooldown) {
                            dialogue_faces[FACE_MENEGHETTI].counter = (dialogue_faces[FACE_MENEGHETTI].counter + 1) % dialogue_faces[FACE_MENEGHETTI].count;
                            dialogue_faces->timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI].frames[dialogue_faces[FACE_MENEGHETTI].counter % dialogue_faces[FACE_MENEGHETTI].count], NULL, &meneghetti_frame);
                    }
                    else {
                        dialogue_faces[FACE_MENEGHETTI].counter = 0;
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI].frames[0], NULL, &meneghetti_frame);
                    }
                }
                break;
            case FACE_MENEGHETTI_ANGRY:
                if (has_faces) {
                    if (!text->waiting_for_input) {
                        while (dialogue_faces->timer >= anim_cooldown) {
                            
                            dialogue_faces[FACE_MENEGHETTI_ANGRY].counter = (dialogue_faces[FACE_MENEGHETTI_ANGRY].counter + 1) % dialogue_faces[FACE_MENEGHETTI_ANGRY].count;
                            dialogue_faces->timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI_ANGRY].frames[dialogue_faces[FACE_MENEGHETTI_ANGRY].counter % dialogue_faces[FACE_MENEGHETTI_ANGRY].count], NULL, &meneghetti_frame);
                    }
                    else {
                        dialogue_faces[FACE_MENEGHETTI_ANGRY].counter = 0;
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI_ANGRY].frames[0], NULL, &meneghetti_frame);
                    }
                }
                break;
            case FACE_MENEGHETTI_SAD:
                if (has_faces) {
                    if (!text->waiting_for_input) {
                        while (dialogue_faces->timer >= anim_cooldown) {
                            
                            dialogue_faces[FACE_MENEGHETTI_SAD].counter = (dialogue_faces[FACE_MENEGHETTI_SAD].counter + 1) % dialogue_faces[FACE_MENEGHETTI_SAD].count;
                            dialogue_faces->timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI_SAD].frames[dialogue_faces[FACE_MENEGHETTI_SAD].counter % dialogue_faces[FACE_MENEGHETTI_SAD].count], NULL, &meneghetti_frame);
                    }
                    else {
                        dialogue_faces[FACE_MENEGHETTI_SAD].counter = 0;
                        SDL_RenderCopy(render, dialogue_faces[FACE_MENEGHETTI_SAD].frames[0], NULL, &meneghetti_frame);
                    }
                }
                break;
            case FACE_PYTHON:
                if (has_faces) {
                    if (!text->waiting_for_input) {
                        while (dialogue_faces->timer >= anim_cooldown) {
                            
                            dialogue_faces[FACE_PYTHON].counter = (dialogue_faces[FACE_PYTHON].counter + 1) % dialogue_faces[FACE_PYTHON].count;
                            dialogue_faces->timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, dialogue_faces[FACE_PYTHON].frames[dialogue_faces[FACE_PYTHON].counter % dialogue_faces[FACE_PYTHON].count], NULL, &python_frame);
                    }
                    else {
                        dialogue_faces[FACE_PYTHON].counter = 0;
                        SDL_RenderCopy(render, dialogue_faces[FACE_PYTHON].frames[0], NULL, &python_frame);
                    }
                }
                break;
            case FACE_CHATGPT:
                if (has_faces) {
                    if (!text->waiting_for_input) {
                        while (dialogue_faces->timer >= anim_cooldown) {
                            
                            dialogue_faces[FACE_CHATGPT].counter = (dialogue_faces[FACE_CHATGPT].counter + 1) % dialogue_faces[FACE_CHATGPT].count;
                            dialogue_faces->timer = 0.0;
                        }
                        
                        SDL_RenderCopy(render, dialogue_faces[FACE_CHATGPT].frames[dialogue_faces[FACE_CHATGPT].counter % dialogue_faces[FACE_CHATGPT].count], NULL, &gpt_frame);
                    }
                    else {
                        dialogue_faces[FACE_CHATGPT].counter = 0;
                        SDL_RenderCopy(render, dialogue_faces[FACE_CHATGPT].frames[0], NULL, &gpt_frame);
                    }
                }
        }
    }
}

void reset_dialogue(Dialogue *text) {
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

void python_attacks(SDL_Renderer *render, Soul *soul, BattleBox battle_box, int *player_health, int damage, int attack_index, Projectile **props, double dt, double turn_timer, Sound *sound, bool clear) {
    static double spawn_timer = 0.0;
    static int objects_spawned = 0;
    static bool attack_active = false;
    
    static Projectile active_objects[15];
    static bool created_object[15] = {false};
    static double objects_speed[15] = {0};

    static double vel_x[15] = {0};
    static double vel_y[15] = {0};
    static double angles[15] = {0};

    static double alpha_counter = 0;

    static bool played_appear_sound = false;

    Mix_Chunk* hit_sound = sound[0].sound;
    Mix_Chunk* appear_sound = sound[1].sound;
    Mix_Chunk* born_sound = sound[2].sound;
    Mix_Chunk* slam_sound = sound[3].sound;
    Mix_Chunk* strike_sound = sound[4].sound;

    if (!clear) {    
        switch(attack_index) {
            case 1:
            case 4:
                if (!attack_active && turn_timer <= 8.0) {
                    if (turn_timer <= 0.8) {
                        props[3][0].collision.x = battle_box.animated_box.x + battle_box.animated_box.w;
                        props[3][0].collision.y = battle_box.animated_box.y;
                        props[3][1].collision.x = battle_box.animated_box.x - props[3][1].collision.w;
                        props[3][1].collision.y = battle_box.animated_box.y + battle_box.animated_box.h - props[3][1].collision.h;
                    }
                    if (attack_index == 4) {
                        alpha_counter += dt * 300;
                        if (alpha_counter >= 255) alpha_counter = 255;
                        SDL_SetTextureAlphaMod(props[3][0].animation.frames[0], alpha_counter);
                        SDL_SetTextureAlphaMod(props[3][1].animation.frames[0], alpha_counter);
                        SDL_SetTextureAlphaMod(props[3][0].animation.frames[1], alpha_counter);
                        SDL_SetTextureAlphaMod(props[3][1].animation.frames[1], alpha_counter);
                        if (!played_appear_sound) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, appear_sound, 0);
                            played_appear_sound = true;
                        }

                        if (props[3][0].collision.x >= battle_box.animated_box.x - 10 || props[3][1].collision.x <= battle_box.animated_box.x - 10) {
                            if (props[3][0].collision.x >= battle_box.animated_box.x - 10) props[3][0].collision.x -= 80.0 * dt;
                            if (props[3][1].collision.x <= battle_box.animated_box.x - 10) props[3][1].collision.x += 80.0 * dt;
                        }
                        else {
                            attack_active = true;
                            spawn_timer = 0.0;
                            objects_spawned = 0;
                        }
                    }
                    else {
                        attack_active = true;
                        spawn_timer = 0.0;
                        objects_spawned = 0;
                    }

                    for (int i = 0; i < 15; i++) {
                        created_object[i] = false;
                    }
                }

                if (attack_index == 4) {
                    props[3][0].texture = animate_sprite(&props[3][0].animation, dt, 0.4, false);
                    props[3][1].texture = animate_sprite(&props[3][1].animation, dt, 0.4, false);

                    SDL_RenderCopyF(render, props[3][0].texture, NULL, &props[3][0].collision);
                    SDL_RenderCopyF(render, props[3][1].texture, NULL, &props[3][1].collision);

                    if (!soul->is_ivulnerable && rects_intersect(&soul->collision, NULL, &props[3][0].collision)) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                        *player_health -= damage;
                        soul->is_ivulnerable = true;
                    }
                    if (!soul->is_ivulnerable && rects_intersect(&soul->collision, NULL, &props[3][1].collision)) {
                        Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                        *player_health -= damage;
                        soul->is_ivulnerable = true;
                    }
                }

                spawn_timer += dt;

                if (attack_active && spawn_timer >= 0.3 && objects_spawned < 15) {
                    for (int i = 0; i < 15; i++) {
                        if (!created_object[i]) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, appear_sound, 0);
                            int random_object = randint(0, 5);
                            active_objects[i] = props[0][random_object];

                            active_objects[i].collision.x = randint(battle_box.animated_box.x, (battle_box.animated_box.x + battle_box.animated_box.w));
                            active_objects[i].collision.y = battle_box.animated_box.y;

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

                        if ((active_objects[i].collision.y + active_objects[i].collision.h) >= (battle_box.animated_box.y + battle_box.animated_box.h)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!soul->is_ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                            *player_health -= damage;
                            soul->is_ivulnerable = true;
                            created_object[i] = false;
                            objects_spawned--;
                            continue;
                        }

                        SDL_RenderCopyExF(render, active_objects[i].texture, NULL, &active_objects[i].collision, 90, NULL, 0);
                    }
                }

                if (turn_timer >= 9.5) {
                    attack_active = false;
                    played_appear_sound = false;
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

                if (attack_active && spawn_timer >= 0.8 && objects_spawned < 6) {
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

                        if (!soul->is_ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                            *player_health -= damage;
                            soul->is_ivulnerable = true;

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

                    props[2][0].collision.x = (battle_box.animated_box.x + (battle_box.animated_box.w / 2)) - (props[2][0].collision.w / 2);
                    props[2][0].collision.y = battle_box.animated_box.y + 10;

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

                        active_objects[i].texture = animate_sprite(&active_objects->animation, dt, 0.2, false);
                            
                        active_objects[i].collision.x += vel_x[i] * dt;
                        active_objects[i].collision.y += vel_y[i] * dt;

                        if (active_objects[i].collision.x < battle_box.animated_box.x + 5 || active_objects[i].collision.x + active_objects[i].collision.w > battle_box.animated_box.x + battle_box.animated_box.w || active_objects[i].collision.y < battle_box.animated_box.y || active_objects[i].collision.y + active_objects[i].collision.h > battle_box.animated_box.y + battle_box.animated_box.h) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, slam_sound, 0);
                            created_object[i] = false;
                            continue;
                        }

                        if (!soul->is_ivulnerable && rects_intersect(&soul->collision, NULL, &active_objects[i].collision)) {
                            Mix_PlayChannel(DEFAULT_CHANNEL, hit_sound, 0);
                            *player_health -= damage;
                            soul->is_ivulnerable = true;
                            created_object[i] = false;
                            continue;
                        }

                        SDL_RenderCopyExF(render, active_objects[i].texture, NULL, &active_objects[i].collision, angles[i] + 90, NULL, 0);
                    }
                }

                if (turn_timer >= 9.5) {
                    attack_active = false;
                    played_appear_sound = false;
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
        alpha_counter = 0.0;

        for (int i = 0; i < 15; i ++) {
            created_object[i] = false;
            objects_speed[i] = 0.0;
            vel_x[i] = 0.0;
            vel_y[i] = 0.0;
            angles[i] = 0.0;
        }
    }
}

void sprite_update(Prop *scenario, Player *player, Animation *animation, double dt, SDL_Rect boxes[], SDL_Rect surfaces[], Sound *sound) {
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

    float move_vel = player->speed * (float)dt;
    int move = (int)roundf(move_vel);

    if (move == 0 && move_vel != 0.0f) move = (move_vel > 0.0f) ? 1 : -1;

    animation->timer += dt;
    const double anim_interval = 0.2;

    bool moving_up = false, moving_down = false, moving_left = false, moving_right = false;

    if (up) {
        player->facing = DIRECTION_UP;
    }
    else if (down) {
        player->facing = DIRECTION_DOWN;
    }
    else if (left) {
        player->facing = DIRECTION_LEFT;
    }
    else if (right) {
        player->facing = DIRECTION_RIGHT;
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
        if (animation->timer >= anim_interval) {
            player->texture = animation[DIRECTION_UP].frames[ player->counters[DIRECTION_UP] % animation[DIRECTION_UP].count ];
            player->counters[DIRECTION_UP] = (player->counters[DIRECTION_UP] + 1) % animation[DIRECTION_UP].count;
            animation->timer = 0.0;
        }
    }
    else if (moving_down) {
        if (animation->timer >= anim_interval) {
            player->texture = animation[DIRECTION_DOWN].frames[ player->counters[DIRECTION_DOWN] % animation[DIRECTION_DOWN].count ];
            player->counters[DIRECTION_DOWN] = (player->counters[DIRECTION_DOWN] + 1) % animation[DIRECTION_DOWN].count;
            animation->timer = 0.0;
        }
    }
    else if (moving_left) {
        if (animation->timer >= anim_interval) {
            player->texture = animation[DIRECTION_LEFT].frames[ player->counters[DIRECTION_LEFT] % animation[DIRECTION_LEFT].count ];
            player->counters[DIRECTION_LEFT] = (player->counters[DIRECTION_LEFT] + 1) % animation[DIRECTION_LEFT].count;
            animation->timer = 0.0;
        }
    }
    else if (moving_right) {
        if (animation->timer >= anim_interval) {
            player->texture = animation[DIRECTION_RIGHT].frames[ player->counters[DIRECTION_RIGHT] % animation[DIRECTION_RIGHT].count ];
            player->counters[DIRECTION_RIGHT] = (player->counters[DIRECTION_RIGHT] + 1) % animation[DIRECTION_RIGHT].count;
            animation->timer = 0.0;
        }
    }
    else {
        player->counters[DIRECTION_UP] = player->counters[DIRECTION_DOWN] = player->counters[DIRECTION_LEFT] = player->counters[DIRECTION_RIGHT] = 0;

        int face = player->facing;
        if (face < 0 || face >= DIRECTION_AMOUNT) face = DIRECTION_DOWN;
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

void update_reflection(Player *original, Player* reflection, Animation *animation) {
    reflection->facing = original->facing;

    int current_frame = original->counters[original->facing];

    reflection->texture = animation[reflection->facing].frames[current_frame];

    reflection->collision.x = original->collision.x;
    reflection->collision.y = original->collision.y + original->collision.h;
    reflection->collision.w = original->collision.w;
    reflection->collision.h = original->collision.h;
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

char *utf8_to_upper(const char *s) {
    if (!s) return NULL;
    setlocale(LC_CTYPE, ""); /* usa a localidade do sistema */

    size_t wlen = mbstowcs(NULL, s, 0);
    if (wlen == (size_t)-1) return strdup(s); /* fallback */

    wchar_t *w = malloc((wlen + 1) * sizeof(wchar_t));
    if (!w) return strdup(s);
    mbstowcs(w, s, wlen + 1);

    for (size_t i = 0; i < wlen; ++i) {
        w[i] = towupper(w[i]);
    }

    size_t outlen = wcstombs(NULL, w, 0);
    if (outlen == (size_t)-1) { free(w); return strdup(s); }

    char *out = malloc(outlen + 1);
    if (!out) { free(w); return strdup(s); }
    wcstombs(out, w, outlen + 1);

    free(w);
    return out;
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
