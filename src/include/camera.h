#ifndef CAMERA_H
#define CAMERA_H

#include <SDL2/SDL.h>
#include "character.h"

typedef struct Camera {
    int x,y;
    int w,h;
} Camera;

void camera_update(Camera *cam, Character *player, int world_w, int world_h);
// void render_with_camera(Camera *cam, SDL_Renderer *renderer,
//                         SDL_Texture *tex, SDL_Rect *src, SDL_Rect *dst_world);

#endif