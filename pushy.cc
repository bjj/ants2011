#include "Bot.h"

using namespace std;

void Bot::pushy()
{
    const Grid<bool> &FOO = enemyThreat;
    const typeof(e_myHills) &BAR = e_myHills;
    vector<Location> targets;

    for (int row = 0; row < state.rows; ++row) {
        bool prev = FOO[row][state.cols - 1];
        bool next = FOO[row][0];
        int prevDist = BAR[row][state.cols - 1];
        int nextDist = BAR[row][0];
        for (int col = 1; col < state.cols; ++col) {
            bool cur = next;
            int dist = nextDist;
            next = FOO[row][col];
            nextDist = BAR[row][col];
            if (cur && !next && dist > nextDist)  // falling, inside && decreasing
                targets.push_back(Location(row, col - 1));
            if (!prev && cur && prevDist < dist)  // rising, inside && increasing
                targets.push_back(Location(row, col - 1));
            prev = cur;
            prevDist = dist;
        }
    }

    for (int col = 0; col < state.cols; ++col) {
        bool prev = FOO[state.rows - 1][col];
        bool next = FOO[0][col];
        int prevDist = BAR[state.rows - 1][col];
        int nextDist = BAR[0][col];
        for (int row = 1; row < state.rows; ++row) {
            bool cur = next;
            int dist = nextDist;
            next = FOO[row][col];
            nextDist = BAR[row][col];
            if (cur && !next && dist > nextDist)  // falling, inside && decreasing
                targets.push_back(Location(row - 1, col));
            if (!prev && cur && prevDist < dist)  // rising, inside && increasing
                targets.push_back(Location(row - 1, col));
            prev = cur;
            prevDist = dist;
        }
    }

    e_push.update(targets.begin(), targets.end());

    vector<Location> intercepts;
    for (int row = 0; row < state.rows; ++row) {
        for (int col = 0; col < state.cols; ++col) {
            if (!state.grid[row][col].isWater && e_push[row][col] == e_self[row][col])
                intercepts.push_back(Location(row, col));
        }
    }

    e_intercept.update(intercepts.begin(), intercepts.end());

#ifdef VISUALIZER
#if 0
    for (uint i = 1; i < 10; ++i) {
        int c = 255 - 40 * (i - 1);
        state.v.setFillColor(c,c,c,0.5);
        for (int row = 0; row < state.rows; ++row)
            for (int col = 0; col < state.cols; ++col) {
                if (e_push[row][col] == int(i))
                    state.v.tile(Location(row,col));
                if (!state.grid[row][col].isWater && e_push[row][col] == e_self[row][col])
                    state.v.tileBorder(Location(row,col), "MM");
            }
    }
#endif
#if 1
    for (uint i = 1; i < 10; ++i) {
        int c = 255 - 40 * (i - 1);
        state.v.setFillColor(c,c,c,0.5);
        for (int row = 0; row < state.rows; ++row)
            for (int col = 0; col < state.cols; ++col) {
                if (e_intercept[row][col] == int(i))
                    state.v.tile(Location(row,col));
            }
    }
#endif
#endif
}
