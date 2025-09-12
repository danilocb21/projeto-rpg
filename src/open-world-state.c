#include "include/state.h"
#include "include/character.h"
#include "include/obstacle.h"
#include "include/camera.h"
#include "include/game.h"
#include <stdio.h>
#include <string.h>

typedef struct OpenWorldCtx {
    Game *game;
    int world_w, world_h;
    SDL_Point origin;
    Camera cam;
    SDL_Texture *background;
    Obstacle *obstacles;
    int obstacles_count;
    Character player;
} OpenWorldCtx;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define FRAME_W 19
#define FRAME_H 32
#define FRAMES_PER_ROW 4

#define OBSTACLES_COUNT 10

static void setup_player_animations(Character *player) {
    for (int dir = 0; dir < 4; ++dir) {
        Animation *a = &player->anims[dir];
        a->frame_count = FRAMES_PER_ROW;
        a->frame_time = 0.12f;
        a->current_frame = 0;
        a->elapsed_time = 0;

        for (int f = 0; f < a->frame_count; ++f) {
            a->frames[f].x = f * FRAME_W;
            a->frames[f].y = dir * FRAME_H;
            a->frames[f].w = FRAME_W;
            a->frames[f].h = FRAME_H;
        }
    }
}

static void open_init(void *vctx) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)vctx;
    Game *game = ctx->game;

    ctx->world_w = SCREEN_WIDTH * 2;
    ctx->world_h = SCREEN_HEIGHT * 2;

    ctx->origin.x = 0;
    ctx->origin.y = -SCREEN_HEIGHT;
    
    ctx->background = createTexture(game->renderer, "assets/sprites/scenario/scenario.png");
    ctx->player.texture = createTexture(game->renderer, "assets/sprites/characters/meneghetti-front.png");
    ctx->player.rect = (SDL_Rect){(SCREEN_WIDTH / 2) - 10, (SCREEN_HEIGHT / 2) - 16, FRAME_W, FRAME_H};
    ctx->player.vel_x = 0;
    ctx->player.vel_y = 0;
    ctx->player.dir = DIR_DOWN;
    ctx->player.moving = false;

    ctx->cam.w = SCREEN_WIDTH;
    ctx->cam.h = SCREEN_HEIGHT;
    ctx->cam.x = 0;
    ctx->cam.y = 0;

    ctx->obstacles_count = OBSTACLES_COUNT;
    ctx->obstacles = (Obstacle*) SDL_malloc(OBSTACLES_COUNT * sizeof(Obstacle));

    const int ox = ctx->origin.x;
    const int oy = ctx->origin.y;

    obstacle_init(&ctx->obstacles[0], game->renderer, (SDL_Rect){ox, oy + 739, 981, 221}, ""); // Bloco inferior esquerdo.
    obstacle_init(&ctx->obstacles[1], game->renderer, (SDL_Rect){ox, oy + 384, 607, 153}, ""); // Bloco superior esquerdo (ponte).
    obstacle_init(&ctx->obstacles[2], game->renderer, (SDL_Rect){ox, oy, 253, 384}, ""); // Bloco ao topo esquerdo.
    obstacle_init(&ctx->obstacles[3], game->renderer, (SDL_Rect){ox + 253, oy, 774, 74}, ""); // Bloco ao topo central.
    obstacle_init(&ctx->obstacles[4], game->renderer, (SDL_Rect){ox + 1027, oy, 253, 384}, ""); // Bloco ao topo direito.
    obstacle_init(&ctx->obstacles[5], game->renderer, (SDL_Rect){ox + 673, oy + 384, 607, 158}, ""); // Bloco superior direito (ponte).
    obstacle_init(&ctx->obstacles[6], game->renderer, (SDL_Rect){ox + 1045, oy + 739, 235, 221}, ""); // Bloco inferior esquerdo.
    obstacle_init(&ctx->obstacles[7], game->renderer, (SDL_Rect){ox + 981, oy + 890, 64, 70}, ""); // Bloco do rodapé (lago).
    obstacle_init(&ctx->obstacles[8], game->renderer, (SDL_Rect){ox + 620, oy + 153, 39, 40}, ""); // Bloco do Mr. Python.
    obstacle_init(&ctx->obstacles[9], game->renderer, (SDL_Rect){ox + 758, oy + 592, 64, 4}, ""); // Bloco da Python Van.

    setup_player_animations(&ctx->player);
}

static void open_handle_event(void *vctx, SDL_Event *event) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)vctx;
    Game *game = ctx->game;
    if (event->type == SDL_QUIT) {
        game->running = false;
    } 
    else if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_ESCAPE) game->running = false;
    }
}

static void open_update(void *vctx, float dt) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)vctx;
    
    Character *player = &ctx->player;
    const Uint8 *kb = SDL_GetKeyboardState(NULL);
    const int speed = 120;
    
    float vx = 0.0f, vy = 0.0f;
    if (kb[SDL_SCANCODE_W] || kb[SDL_SCANCODE_UP]) vy = -1.0f;
    if (kb[SDL_SCANCODE_S] || kb[SDL_SCANCODE_DOWN]) vy = 1.0f;
    if (kb[SDL_SCANCODE_A] || kb[SDL_SCANCODE_LEFT]) vx = -1.0f;
    if (kb[SDL_SCANCODE_D] || kb[SDL_SCANCODE_RIGHT]) vx = 1.0f;

    if (vx != 0 && vy != 0) { 
        const float inv = 0.70710678f; vx *= inv;
        vy *= inv; 
    }
    player->vel_x = vx * speed;
    player->vel_y = vy * speed;

    // Ta parado
    if (vx == 0.0f && vy == 0.0f) {
        character_set_direction(player, player->dir, false);
    }
    // Se mexendo
    else if (fabsf(vx) > fabsf(vy)) {
        if (vx > 0) character_set_direction(player, DIR_RIGHT, true);
        else character_set_direction(player, DIR_LEFT, true);
    }
    else {
        if (vy > 0) character_set_direction(player, DIR_DOWN, true);
        else character_set_direction(player, DIR_UP, true);
    }
    sprite_update(player, dt);
    camera_update(&ctx->cam, &ctx->player, ctx->world_w, ctx->world_h);
}

static void open_render(void *vctx) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)vctx;
    Game *game = ctx->game;

    SDL_RenderClear(game->renderer);


    int tex_w = 0, tex_h = 0;
    SDL_QueryTexture(ctx->background, NULL, NULL, &tex_w, &tex_h);

    // garante que a src rect fique dentro dos limites da textura
    int view_w = ctx->cam.w > tex_w ? tex_w : ctx->cam.w;
    int view_h = ctx->cam.h > tex_h ? tex_h : ctx->cam.h;

    int max_x = tex_w - view_w;
    int max_y = tex_h - view_h;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;

    int src_x = ctx->cam.x;
    if (src_x < 0) src_x = 0;
    else if (src_x > max_x) src_x = max_x;

    int src_y = ctx->cam.y;
    if (src_y < 0) src_y = 0;
    else if (src_y > max_y) src_y = max_y;

    SDL_Rect src_bg = { src_x, src_y, view_w, view_h };
    SDL_Rect dst_bg = { 0, 0, view_w, view_h };

    SDL_RenderCopy(game->renderer, ctx->background, &src_bg, &dst_bg);
    
    SDL_Rect src = get_current_frame(&ctx->player);
    SDL_Rect dst_screen = {
        ctx->player.rect.x - ctx->cam.x,
        ctx->player.rect.y - ctx->cam.y,
        ctx->player.rect.w,
        ctx->player.rect.h
    };
    SDL_RenderCopy(game->renderer, ctx->player.texture, &src, &dst_screen);
    SDL_RenderPresent(game->renderer);
}

static void open_cleanup(void *vctx) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)vctx;
    object_cleanup(ctx->background);
    object_cleanup(ctx->player.texture);
    if (ctx->obstacles) {
        for (int i = 0; i < ctx->obstacles_count; ++i) {
            if (ctx->obstacles[i].texture) 
                object_cleanup(ctx->obstacles[i].texture);
        }
        SDL_free(ctx->obstacles);
        ctx->obstacles = NULL;
        ctx->obstacles_count = 0;
    }
}

State* create_open_world_state(Game *game) {
    OpenWorldCtx *ctx = (OpenWorldCtx*)SDL_malloc(sizeof(OpenWorldCtx));
    memset(ctx, 0, sizeof(OpenWorldCtx));
    ctx->game = game;
    State *s = (State*)SDL_malloc(sizeof(State));
    s->init = open_init;
    s->handle_event = open_handle_event;
    s->update = open_update;
    s->render = open_render;
    s->cleanup = open_cleanup;
    s->ctx = ctx;
    return s;
}