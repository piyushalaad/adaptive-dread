// Adaptive Dread - entry point
// Milestone 1: tile map with walls and player collision.

#include "raylib.h"
#include "map.h"

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

    // Start in the top-left room (tile 2,2), clear of any walls
    Player player = {
        .position = { 2 * TILE_SIZE + 4, 2 * TILE_SIZE + 4 },
        .speed = PLAYER_SPEED
    };

    // Game loop: update -> draw, once per frame
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();   // seconds since last frame
        UpdatePlayer(&player, dt);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawMap();
        DrawRectangleV(player.position, (Vector2){ PLAYER_SIZE, PLAYER_SIZE }, (Color){ 200, 40, 40, 255 });
        DrawText("Adaptive Dread - WASD to move", 44, 40, 20, LIGHTGRAY);
        DrawFPS(SCREEN_WIDTH - 110, 40);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
