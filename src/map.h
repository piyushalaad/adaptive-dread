#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include "raylib.h"

#define TILE_SIZE 32
#define MAP_COLS  40   // 40 * 32 = 1280 px wide
#define MAP_ROWS  22   // 22 * 32 = 704 px tall

// True if the tile at (col, row) is a wall. Anything outside the map counts as a wall.
bool IsWallAt(int col, int row);

// True if any part of the rectangle overlaps a wall tile.
bool RectHitsWall(Rectangle rect);

// Draw every wall and floor tile.
void DrawMap(void);

#endif
