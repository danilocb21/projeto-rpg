#ifndef CHARACTER_H
#define CHARACTER_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum Direction {
    DIR_DOWN = 0,
    DIR_LEFT = 1,
    DIR_RIGHT = 2,
    DIR_UP = 3
} Direction;

typedef struct Animation {
    SDL_Rect frames[16];
    int frame_count;
    int current_frame;
    float frame_time;
    float elapsed_time;
} Animation;

typedef struct Character {
    SDL_Texture *texture;
    SDL_Rect rect; // dest rect
    float vel_x, vel_y;
    Animation anims[4];
    Direction dir;
    bool moving;
} Character;

void sprite_update(Character *ch, float dt);
SDL_Rect get_current_frame(Character *ch);
void character_set_direction(Character *ch, Direction dir, bool moving);
bool check_collision(SDL_Rect a, SDL_Rect b);
bool circle_collision(SDL_Rect a, SDL_Rect b);

#endif