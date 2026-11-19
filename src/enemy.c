#include <math.h>
#include "enemy.h"

static Vector2 EnemyCenter(const Enemy *enemy)
{
    return (Vector2){ enemy->position.x + ENEMY_SIZE / 2.0f, enemy->position.y + ENEMY_SIZE / 2.0f };
}

Rectangle EnemyRect(const Enemy *enemy)
{
    return (Rectangle){ enemy->position.x, enemy->position.y, ENEMY_SIZE, ENEMY_SIZE };
}

void InitEnemy(Enemy *enemy, Vector2 position)
{
    enemy->position = position;
    enemy->speed = ENEMY_SPEED;
    enemy->path.length = 0;
    enemy->path.nodesExplored = 0;
    enemy->pathIndex = 0;
    enemy->repathTimer = 0.0f;   // search on the very first frame
    enemy->lastSearchMs = 0.0;
}

// Move toward `target` without walking into walls (same axis-by-axis idea as the player)
static void MoveToward(Enemy *enemy, Vector2 target, float dt)
{
    Vector2 center = EnemyCenter(enemy);
    float dx = target.x - center.x;
    float dy = target.y - center.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) return;

    float step = enemy->speed * dt;
    if (step > dist) step = dist;   // don't overshoot the target

    Rectangle next = EnemyRect(enemy);

    next.x += dx / dist * step;
    if (!RectHitsWall(next)) enemy->position.x = next.x;
    else next.x = enemy->position.x;

    next.y += dy / dist * step;
    if (!RectHitsWall(next)) enemy->position.y = next.y;
}

void UpdateEnemy(Enemy *enemy, Vector2 playerCenter, float dt)
{
    // Re-run A* every so often instead of every frame -- the player moves,
    // so the path goes stale, but searching 60 times a second is wasteful.
    enemy->repathTimer -= dt;
    if (enemy->repathTimer <= 0.0f) {
        enemy->repathTimer = ENEMY_REPATH_TIME;

        double t0 = GetTime();
        bool found = FindPath(TileFromPixel(EnemyCenter(enemy)), TileFromPixel(playerCenter), &enemy->path);
        enemy->lastSearchMs = (GetTime() - t0) * 1000.0;

        enemy->pathIndex = 1;   // index 0 is the tile we're already standing on
        if (!found) enemy->path.length = 0;
    }

    if (enemy->path.length == 0) return;   // no route to the player: wait

    if (enemy->pathIndex < enemy->path.length) {
        // Walk toward the center of the next tile on the path
        Vector2 target = TileCenter(enemy->path.tiles[enemy->pathIndex]);
        Vector2 center = EnemyCenter(enemy);
        float dx = target.x - center.x, dy = target.y - center.y;

        if (dx * dx + dy * dy < 4.0f) enemy->pathIndex++;   // within 2px: next tile
        else MoveToward(enemy, target, dt);
    } else {
        // Already in the player's tile: close the last gap directly
        MoveToward(enemy, playerCenter, dt);
    }
}

void DrawEnemy(const Enemy *enemy, bool showPath)
{
    if (showPath && enemy->path.length > 0) {
        Vector2 prev = EnemyCenter(enemy);
        for (int i = enemy->pathIndex; i < enemy->path.length; i++) {
            Vector2 p = TileCenter(enemy->path.tiles[i]);
            DrawLineV(prev, p, (Color){ 90, 200, 120, 160 });
            DrawCircleV(p, 3.0f, (Color){ 90, 200, 120, 220 });
            prev = p;
        }
    }

    DrawRectangleRec(EnemyRect(enemy), (Color){ 110, 160, 90, 255 });
}
