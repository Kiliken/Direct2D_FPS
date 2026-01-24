#pragma once
#include <vector>
#include <cmath>
#include <d2d1.h>
#include "raycastTest.h" // for getTile, mapWidth, mapHeight

struct IPoint {
    int x = 0;
    int y = 0;
    bool operator==(const IPoint& o) const { return x == o.x && y == o.y; }
    bool operator!=(const IPoint& o) const { return !(*this == o); }
};

inline bool InBounds(int tx, int ty) {
    return tx >= 0 && ty >= 0 && tx < mapWidth && ty < mapHeight;
}

inline bool IsWalkable(int tx, int ty) {
    if (!InBounds(tx, ty)) return false;
    return getTile(tx, ty) == '.';
}

inline IPoint WorldToTile(const D2D_POINT_2F& p) {
    return { (int)std::floor(p.x), (int)std::floor(p.y) };
}

inline D2D_POINT_2F TileCenter(const IPoint& t) {
    return { (float)t.x + 0.5f, (float)t.y + 0.5f };
}

// Returns tile path including start and goal. Empty if no path.
std::vector<IPoint> AStarTilePath(const IPoint& start, const IPoint& goal);