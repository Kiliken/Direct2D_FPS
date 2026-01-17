#include "Pathfinding.h"
#include <queue>
#include <limits>
#include <algorithm>


static inline int Heuristic(const IPoint& a, const IPoint& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y); // Manhattan
}


struct PQItem {
    int idx;
    int f;
    bool operator<(const PQItem& other) const { return f > other.f; } // min-heap
};


static inline int ToIndex(int x, int y) { return y * mapWidth + x; }


std::vector<IPoint> AStarTilePath(const IPoint& start, const IPoint& goal) {
    if (!InBounds(start.x, start.y) || !InBounds(goal.x, goal.y)) return {};
    if (!IsWalkable(start.x, start.y) || !IsWalkable(goal.x, goal.y)) return {};

    const int N = mapWidth * mapHeight;
    std::vector<int> gScore(N, (std::numeric_limits<int>::max)());
    std::vector<int> cameFrom(N, -1);
    std::vector<char> closed(N, 0);

    std::priority_queue<PQItem> open;

    const int startIdx = ToIndex(start.x, start.y);
    const int goalIdx = ToIndex(goal.x, goal.y);

    gScore[startIdx] = 0;
    open.push({ startIdx, Heuristic(start, goal) });

    while (!open.empty()) {
        auto cur = open.top(); open.pop();
        if (closed[cur.idx]) continue;
        closed[cur.idx] = 1;

        if (cur.idx == goalIdx) {
            // reconstruct
            std::vector<IPoint> path;
            for (int idx = cur.idx; idx != -1; idx = cameFrom[idx]) {
                int cx = idx % mapWidth;
                int cy = idx / mapWidth;
                path.push_back({ cx, cy });
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        int cx = cur.idx % mapWidth;
        int cy = cur.idx / mapWidth;

        static const int DX[4] = { 1, -1, 0, 0 };
        static const int DY[4] = { 0, 0, 1, -1 };
        for (int i = 0; i < 4; ++i) {
            int nx = cx + DX[i];
            int ny = cy + DY[i];
            if (!InBounds(nx, ny) || !IsWalkable(nx, ny)) continue;
            int nIdx = ToIndex(nx, ny);
            if (closed[nIdx]) continue;
            int tentativeG = gScore[cur.idx] + 1;
            if (tentativeG < gScore[nIdx]) {
                cameFrom[nIdx] = cur.idx;
                gScore[nIdx] = tentativeG;
                int h = Heuristic({ nx, ny }, goal);
                open.push({ nIdx, tentativeG + h });
            }
        }
    }
    return {};
}