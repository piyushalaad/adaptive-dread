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
