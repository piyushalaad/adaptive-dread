#ifndef ENEMY_H
#define ENEMY_H

#include <stdbool.h>
#include "raylib.h"
#include "pathfinding.h"

#define ENEMY_SIZE        24.0f
#define ENEMY_SPEED       140.0f   // slower than the player, so you can escape
#define ENEMY_REPATH_TIME 0.25f    // seconds between A* searches

typedef struct {
    Vector2 position;      // top-left corner, in pixels
    float speed;
    Path path;             // current route to the player
    int pathIndex;         // which tile in the path we're walking toward
    float repathTimer;     // counts down to the next A* search
    double lastSearchMs;   // how long the last A* search took (milliseconds)
} Enemy;

void InitEnemy(Enemy *enemy, Vector2 position);
void UpdateEnemy(Enemy *enemy, Vector2 playerCenter, float dt);
void DrawEnemy(const Enemy *enemy, bool showPath);
Rectangle EnemyRect(const Enemy *enemy);

#endif
