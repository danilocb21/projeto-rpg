#include "include/camera.h"

void camera_update(Camera *cam, Character *player, int world_w, int world_h) {
    // Centraliza no player
    cam->x = player->rect.x + player->rect.w/2 - cam->w/2;
    cam->y = player->rect.y + player->rect.h/2 - cam->h/2;

    // Limites do mundo (não deixar a câmera sair do mapa)
    if (cam->x < 0) cam->x = 0;
    if (cam->y < 0) cam->y = 0;
    if (cam->x > world_w - cam->w) cam->x = world_w - cam->w;
    if (cam->y > world_h - cam->h) cam->y = world_h - cam->h;
}

// void camera_offset(Camera *cam, SDL_Renderer *renderer, SDL_Texture *tex, SDL_Rect *src, SDL_Rect *dst_world) {
//     SDL_Rect dst_screen = {
//         dst_world->x - cam->x,
//         dst_world->y - cam->y,
//         dst_world->w,
//         dst_world->h
//     };
//     SDL_RenderCopy(renderer, tex, src, &dst_screen);
// }