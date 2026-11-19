// Adaptive Dread - entry point
// Milestone 2: an enemy that chases the player using A* pathfinding.

#include <stdbool.h>
#include "raylib.h"
#include "map.h"
#include "enemy.h"

#define SCREEN_WIDTH  (MAP_COLS * TILE_SIZE)
#define SCREEN_HEIGHT (MAP_ROWS * TILE_SIZE)
#define PLAYER_SIZE   24.0f    // a bit smaller than a tile so the player fits through doorways
#define PLAYER_SPEED  220.0f   // pixels per second

typedef struct {
    Vector2 position;
    float speed;
} Player;

static Rectangle PlayerRect(Vector2 pos)
{
    return (Rectangle){ pos.x, pos.y, PLAYER_SIZE, PLAYER_SIZE };
}

static Vector2 PlayerCenter(const Player *player)
{
    return (Vector2){ player->position.x + PLAYER_SIZE / 2.0f, player->position.y + PLAYER_SIZE / 2.0f };
}

static void UpdatePlayer(Player *player, float dt)
{
    Vector2 dir = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) dir.y += 1.0f;
    if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) dir.x += 1.0f;

    // Normalize so diagonal movement isn't faster than straight movement
    if (dir.x != 0.0f && dir.y != 0.0f) {
        dir.x *= 0.7071f;
        dir.y *= 0.7071f;
    }

    // Move one axis at a time. If a move would put us inside a wall, cancel only
    // that axis -- this lets the player slide along walls instead of sticking.
    Vector2 next = player->position;

    next.x += dir.x * player->speed * dt;
    if (!RectHitsWall(PlayerRect(next))) player->position.x = next.x;
    else next.x = player->position.x;

    next.y += dir.y * player->speed * dt;
    if (!RectHitsWall(PlayerRect(next))) player->position.y = next.y;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Adaptive Dread");
    SetTargetFPS(60);

    // Player starts in the top-left room (tile 2,2)
    Player player = {
        .position = { 2 * TILE_SIZE + 4, 2 * TILE_SIZE + 4 },
        .speed = PLAYER_SPEED
    };

    // Enemy starts in the bottom-right room (tile 36,18)
    Enemy enemy;
    InitEnemy(&enemy, (Vector2){ 36 * TILE_SIZE + 4, 18 * TILE_SIZE + 4 });

    bool showPath = true;   // TAB toggles the A* debug view

    // Game loop: update -> draw, once per frame
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();   // seconds since last frame

        if (IsKeyPressed(KEY_TAB)) showPath = !showPath;
        if (IsKeyPressed(KEY_H))   SetUseHeuristic(!GetUseHeuristic());

        UpdatePlayer(&player, dt);
        UpdateEnemy(&enemy, PlayerCenter(&player), dt);

        bool caught = CheckCollisionRecs(PlayerRect(player.position), EnemyRect(&enemy));

        BeginDrawing();
        ClearBackground(BLACK);
        DrawMap();
        DrawEnemy(&enemy, showPath);
        DrawRectangleV(player.position, (Vector2){ PLAYER_SIZE, PLAYER_SIZE }, (Color){ 200, 40, 40, 255 });

        DrawText("WASD: move   TAB: toggle path   H: toggle heuristic", 44, 40, 20, LIGHTGRAY);
        DrawText(TextFormat("%s: %d tiles explored, path %d tiles, %.3f ms",
                            GetUseHeuristic() ? "A*" : "Dijkstra (h = 0)",
                            enemy.path.nodesExplored, enemy.path.length, enemy.lastSearchMs),
                 44, 64, 18, GetUseHeuristic() ? GRAY : ORANGE);
        DrawFPS(SCREEN_WIDTH - 110, 40);

        if (caught) {
            const char *msg = "CAUGHT!";
            DrawText(msg, SCREEN_WIDTH / 2 - MeasureText(msg, 60) / 2, SCREEN_HEIGHT / 2 - 30, 60, RED);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
