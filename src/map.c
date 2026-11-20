#include <math.h>
#include "map.h"

// '#' = wall, '.' = floor. Each string is one row of the map.
static const char *LEVEL[MAP_ROWS] = {
    "########################################",
    "#..........#.................#.........#",
    "#..........#.................#.........#",
    "#..........#.................#.........#",
    "#..........#......####.......#.........#",
    "#.................####.................#",
    "#.................####.................#",
    "#..........#.................#.........#",
    "#..........#.................#.........#",
    "######.#####.................#####.#####",
    "#............................#.........#",
    "#............................#.........#",
    "#.....##########.............#.........#",
    "#.....#......................#.........#",
    "#.....#................................#",
    "#.....#................................#",
    "#.....#..........######......#.........#",
    "#................#....#......#.........#",
    "#................#....#......#.........#",
    "#................#...........#.........#",
    "#............................#.........#",
    "########################################",
};

bool IsWallAt(int col, int row)
{
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return true;
    return LEVEL[row][col] == '#';
}

bool RectHitsWall(Rectangle rect)
{
    // Convert the rectangle's edges from pixels to tile coordinates.
    // The -0.001f keeps a rect that ends exactly on a tile edge from counting the next tile.
    int left   = (int)floorf(rect.x / TILE_SIZE);
    int right  = (int)floorf((rect.x + rect.width  - 0.001f) / TILE_SIZE);
    int top    = (int)floorf(rect.y / TILE_SIZE);
    int bottom = (int)floorf((rect.y + rect.height - 0.001f) / TILE_SIZE);

    // Check every tile the rectangle touches.
    for (int row = top; row <= bottom; row++) {
        for (int col = left; col <= right; col++) {
            if (IsWallAt(col, row)) return true;
        }
    }
    return false;
}

#define RAY_STEP 4.0f   // pixels between samples; smaller than a tile, so walls can't be skipped

Vector2 CastRay(Vector2 from, Vector2 to)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dist = sqrtf(dx * dx + dy * dy);
    int steps = (int)(dist / RAY_STEP);

    for (int i = 1; i <= steps; i++) {
        float t = (i * RAY_STEP) / dist;
        Vector2 p = { from.x + dx * t, from.y + dy * t };
        if (IsWallAt((int)floorf(p.x / TILE_SIZE), (int)floorf(p.y / TILE_SIZE))) return p;
    }
    return to;
}

bool HasLineOfSight(Vector2 from, Vector2 to)
{
    Vector2 hit = CastRay(from, to);
    return hit.x == to.x && hit.y == to.y;
}

void DrawMap(void)
{
    for (int row = 0; row < MAP_ROWS; row++) {
        for (int col = 0; col < MAP_COLS; col++) {
            int x = col * TILE_SIZE;
            int y = row * TILE_SIZE;

            if (IsWallAt(col, row)) {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){ 70, 70, 80, 255 });
            } else {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){ 25, 25, 30, 255 });
                // Faint grid lines so you can see the tiles (handy for debugging A* later)
                DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, (Color){ 35, 35, 42, 255 });
            }
        }
    }
}
