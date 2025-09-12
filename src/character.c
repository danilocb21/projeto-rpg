#include "include/character.h"
#include <math.h>

void sprite_update(Character *ch, float dt) {
    ch->rect.x += (int)(ch->vel_x * dt);
    ch->rect.y += (int)(ch->vel_y * dt);
    
    Animation *anim = &ch->anims[ch->dir];
    if (anim->frame_count <= 0) return;

    if (!ch->moving) {
        anim->current_frame = 0;
        anim->elapsed_time = 0;
        return; 
    }

    anim->elapsed_time += dt;
    if (anim->elapsed_time >= anim->frame_time) {
        anim->current_frame = (anim->current_frame + 1) % anim->frame_count;
        anim->elapsed_time = 0;
    }
}

SDL_Rect get_current_frame(Character *ch) {
    Animation *anim = &ch->anims[ch->dir];
    if (anim->frame_count <= 0) return (SDL_Rect){0,0, ch->rect.w, ch->rect.h};

    return anim->frames[anim->current_frame];
}

void character_set_direction(Character *ch, Direction dir, bool moving) {
    if (dir < DIR_DOWN || dir > DIR_UP) return;

    ch->dir = dir;
    ch->moving = moving;
    Animation *anim = &ch->anims[dir];
    if (ch->moving) {
        anim->elapsed_time = 0;
        anim->current_frame = 0;
    }
    else {
        anim->current_frame = 0;
        anim->elapsed_time = 0;
    }
}

bool check_collision(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

bool circle_collision(SDL_Rect a, SDL_Rect b) {
    int ax = a.x + a.w/2;
    int ay = a.y + a.h/2;
    int bx = b.x + b.w/2;
    int by = b.y + b.h/2;
    int dx = ax - bx;
    int dy = ay - by;
    int dist2 = dx*dx + dy*dy;
    int ra = (a.w > a.h ? a.w : a.h) / 2;
    int rb = (b.w > b.h ? b.w : b.h) / 2;
    int r = ra + rb;
    return dist2 <= r*r;
}