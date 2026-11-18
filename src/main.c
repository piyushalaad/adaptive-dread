// Adaptive Dread - entry point
// Milestone 0: open a window and move the player around with WASD.

#include "raylib.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define PLAYER_SIZE   32.0f
#define PLAYER_SPEED  220.0f   // pixels per second

typedef struct {
    Vector2 position;
    float speed;
} Player;

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

    // Multiply by dt so speed is the same at any frame rate
    player->position.x += dir.x * player->speed * dt;
    player->position.y += dir.y * player->speed * dt;

    // Keep the player on screen
    if (player->position.x < 0) player->position.x = 0;
    if (player->position.y < 0) player->position.y = 0;
    if (player->position.x > SCREEN_WIDTH - PLAYER_SIZE)  player->position.x = SCREEN_WIDTH - PLAYER_SIZE;
    if (player->position.y > SCREEN_HEIGHT - PLAYER_SIZE) player->position.y = SCREEN_HEIGHT - PLAYER_SIZE;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Adaptive Dread");
    SetTargetFPS(60);

    Player player = {
        .position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
        .speed = PLAYER_SPEED
    };

    // Game loop: update -> draw, once per frame
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();   // seconds since last frame
        UpdatePlayer(&player, dt);

        BeginDrawing();
        ClearBackground((Color){ 15, 15, 20, 255 });
        DrawRectangleV(player.position, (Vector2){ PLAYER_SIZE, PLAYER_SIZE }, (Color){ 200, 40, 40, 255 });
        DrawText("Adaptive Dread - WASD to move", 20, 20, 20, LIGHTGRAY);
        DrawFPS(SCREEN_WIDTH - 100, 20);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
