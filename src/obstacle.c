#include "include/obstacle.h"

void obstacle_init(Obstacle *obst, SDL_Renderer *renderer, SDL_Rect coords, const char *file) {
    obst->texture = createTexture(renderer, file);
    obst->rect = coords;
    obst->color = (SDL_Color){255, 255, 255, 255};
}

void obstacle_render(Obstacle *obst, SDL_Renderer *renderer, int cam_x, int cam_y) {
    if (!obst || !renderer) return;

    SDL_Rect dst = {
        obst->rect.x - cam_x,
        obst->rect.y - cam_y,
        obst->rect.w,
        obst->rect.h
    };

    if (obst->texture) {
        SDL_RenderCopy(renderer, obst->texture, NULL, &dst);
    } 
    else {
        SDL_SetRenderDrawColor(renderer, obst->color.r, obst->color.g, obst->color.b, obst->color.a);
        SDL_RenderFillRect(renderer, &dst);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderDrawRect(renderer, &dst);
    }
}