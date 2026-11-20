// Adaptive Dread - entry point
// Milestone 3: enemy state machine (patrol -> investigate -> chase -> attack),
// vision cones with line of sight, hearing, and player health.

#include <stdbool.h>
#include "raylib.h"
#include "map.h"
#include "enemy.h"

#define SCREEN_WIDTH  (MAP_COLS * TILE_SIZE)
#define SCREEN_HEIGHT (MAP_ROWS * TILE_SIZE)

#define PLAYER_SIZE         24.0f
#define PLAYER_WALK_SPEED   160.0f
#define PLAYER_SPRINT_SPEED 260.0f
#define NOISE_WALK          (2 * TILE_SIZE)   // footsteps carry 2 tiles when walking...
#define NOISE_SPRINT        (7 * TILE_SIZE)   // ...and 7 tiles when sprinting
#define PLAYER_MAX_HEALTH   5
#define HIT_FLASH_TIME      0.25f

#define ENEMY_COUNT 3

typedef struct {
    Vector2 position;
    int health;
    float noiseRadius;   // recalculated every frame from how the player is moving
    float hitFlash;      // > 0 right after taking damage
} Player;

// Patrol routes (tile coordinates). Each enemy loops through its waypoints.
static const Tile ROUTE_TOP[]    = { {14, 2}, {26, 2}, {26, 7}, {14, 7} };
static const Tile ROUTE_LEFT[]   = { { 3,11}, { 3,19}, {14,19}, {14,14} };
static const Tile ROUTE_RIGHT[]  = { {33,11}, {37,11}, {37,19}, {32,19} };

static Rectangle PlayerRect(Vector2 pos)
{
    return (Rectangle){ pos.x, pos.y, PLAYER_SIZE, PLAYER_SIZE };
}

static Vector2 PlayerCenter(const Player *player)
{
    return (Vector2){ player->position.x + PLAYER_SIZE / 2.0f, player->position.y + PLAYER_SIZE / 2.0f };
}

static void ResetGame(Player *player, Enemy enemies[])
{
    player->position = (Vector2){ 4 * TILE_SIZE + 4, 5 * TILE_SIZE + 4 };   // top-left room, clear of the HUD text
    player->health = PLAYER_MAX_HEALTH;
    player->noiseRadius = 0.0f;
    player->hitFlash = 0.0f;

    InitEnemy(&enemies[0], ROUTE_TOP,   4);
    InitEnemy(&enemies[1], ROUTE_LEFT,  4);
    InitEnemy(&enemies[2], ROUTE_RIGHT, 4);
}

static void UpdatePlayer(Player *player, float dt)
{
    Vector2 dir = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) dir.y += 1.0f;
    if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) dir.x += 1.0f;

    bool moving = (dir.x != 0.0f || dir.y != 0.0f);
    bool sprinting = moving && IsKeyDown(KEY_LEFT_SHIFT);
    float speed = sprinting ? PLAYER_SPRINT_SPEED : PLAYER_WALK_SPEED;

    // Standing still is silent, walking is quiet, sprinting is loud
    player->noiseRadius = !moving ? 0.0f : (sprinting ? NOISE_SPRINT : NOISE_WALK);

    // Normalize so diagonal movement isn't faster than straight movement
    if (dir.x != 0.0f && dir.y != 0.0f) {
        dir.x *= 0.7071f;
        dir.y *= 0.7071f;
    }

    // Move one axis at a time so the player slides along walls
    Vector2 next = player->position;

    next.x += dir.x * speed * dt;
    if (!RectHitsWall(PlayerRect(next))) player->position.x = next.x;
    else next.x = player->position.x;

    next.y += dir.y * speed * dt;
    if (!RectHitsWall(PlayerRect(next))) player->position.y = next.y;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Adaptive Dread");
    SetTargetFPS(60);

    Player player;
    Enemy enemies[ENEMY_COUNT];
    ResetGame(&player, enemies);

    bool debug = true;   // TAB: vision cones, paths, state labels, noise ring

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        bool dead = player.health <= 0;

        if (IsKeyPressed(KEY_TAB)) debug = !debug;
        if (IsKeyPressed(KEY_H))   SetUseHeuristic(!GetUseHeuristic());
        if (IsKeyPressed(KEY_R))   { ResetGame(&player, enemies); dead = false; }

        // ---------- Update ----------
        if (!dead) {
            UpdatePlayer(&player, dt);

            PlayerSense sense = { PlayerCenter(&player), player.noiseRadius };
            for (int i = 0; i < ENEMY_COUNT; i++) {
                int damage = UpdateEnemy(&enemies[i], sense, dt);
                if (damage > 0) {
                    player.health -= damage;
                    player.hitFlash = HIT_FLASH_TIME;
                }
            }
        }
        if (player.hitFlash > 0.0f) player.hitFlash -= dt;

        // ---------- Draw ----------
        BeginDrawing();
        ClearBackground(BLACK);
        DrawMap();

        if (debug && player.noiseRadius > 0.0f) {
            DrawCircleLinesV(PlayerCenter(&player), player.noiseRadius, (Color){ 120, 160, 255, 90 });
        }

        for (int i = 0; i < ENEMY_COUNT; i++) DrawEnemy(&enemies[i], debug);

        DrawRectangleV(player.position, (Vector2){ PLAYER_SIZE, PLAYER_SIZE }, (Color){ 200, 40, 40, 255 });

        // HUD
        DrawText("WASD: move  SHIFT: sprint (loud)  TAB: debug  H: heuristic  R: restart", 44, 40, 18, LIGHTGRAY);

        int explored = 0;
        double worstMs = 0.0;
        for (int i = 0; i < ENEMY_COUNT; i++) {
            explored += enemies[i].path.nodesExplored;
            if (enemies[i].lastSearchMs > worstMs) worstMs = enemies[i].lastSearchMs;
        }
        DrawText(TextFormat("%s: %d tiles explored (all enemies), slowest search %.3f ms",
                            GetUseHeuristic() ? "A*" : "Dijkstra (h = 0)", explored, worstMs),
                 44, 62, 16, GetUseHeuristic() ? GRAY : ORANGE);

        // Health: one square per hit point, top-right under the FPS counter
        int hpX = SCREEN_WIDTH - 44 - PLAYER_MAX_HEALTH * 22;
        DrawText("HP", hpX - 30, 64, 16, LIGHTGRAY);
        for (int i = 0; i < PLAYER_MAX_HEALTH; i++) {
            Color c = i < player.health ? (Color){ 200, 40, 40, 255 } : (Color){ 60, 60, 60, 255 };
            DrawRectangle(hpX + i * 22, 64, 16, 16, c);
        }
        DrawFPS(SCREEN_WIDTH - 110, 40);

        if (player.hitFlash > 0.0f) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 255, 0, 0, (unsigned char)(player.hitFlash / HIT_FLASH_TIME * 90) });
        }

        if (player.health <= 0) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 170 });
            const char *msg = "YOU DIED";
            DrawText(msg, SCREEN_WIDTH / 2 - MeasureText(msg, 60) / 2, SCREEN_HEIGHT / 2 - 40, 60, RED);
            const char *sub = "Press R to restart";
            DrawText(sub, SCREEN_WIDTH / 2 - MeasureText(sub, 20) / 2, SCREEN_HEIGHT / 2 + 30, 20, LIGHTGRAY);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
