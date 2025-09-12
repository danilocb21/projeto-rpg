#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <SDL2/SDL.h>
#include "game.h"

typedef struct Obstacle {
    SDL_Rect rect;
    SDL_Texture *texture;
    SDL_Color color;       // cor usada se texture == NULL
} Obstacle;

void obstacle_init(Obstacle *obst, SDL_Renderer *renderer, SDL_Rect coords, const char *file);
void obstacle_render(Obstacle *obst, SDL_Renderer *renderer, int cam_x, int cam_y);

#endif