#include "Bot.h"

using namespace std;

void Bot::pushy()
{
    e_push.update(state.enemyAnts.begin(), state.enemyAnts.end());
    UniEdt e_selfToward("toward", e_myHills);
    e_selfToward.update(state.myAnts.begin(), state.myAnts.end());

    tbonus.fill(0);

    for (int row = 0; row < state.rows; ++row) {
        for (int col = 0; col < state.cols; ++col) {
            if (state.grid[row][col].isWater)
                continue;
            if (e_push[row][col] < e_selfToward[row][col])
                tbonus[row][col] += max(2, min(6, 10 - e_push[row][col]));
            if (e_frontline[row][col] < e_selfToward[row][col])
                tbonus[row][col] += max(1, min(3, 10 - e_frontline[row][col]));
        }
    }

    Grid<int> diffuse;
    for (int i = 0; i < 2; ++i) {
        diffuse.fill(0);
        for (int row = 0; row < state.rows; ++row) {
            for (int col = 0; col < state.cols; ++col) {
                if (tbonus[row][col] == 0) {
                    int total = 0;
                    for (int d = 0; d < TDIRECTIONS; ++d)
                        total += tbonus(state.getLocation(Location(row, col), d));
                    diffuse[row][col] = total / TDIRECTIONS / 2;
                } else {
                    diffuse[row][col] = tbonus[row][col];
                }
            }
        }
        tbonus.swap(diffuse);
    }

#ifdef VISUALIZER
#if 1
    for (uint i = 1; i < 10; ++i) {
        int c = 255 - 40 * (i - 1);
        state.v.setFillColor(c,c,c,0.5);
        for (int row = 0; row < state.rows; ++row)
            for (int col = 0; col < state.cols; ++col) {
                if (tbonus[row][col] == int(i))
                    state.v.tile(Location(row,col));
            }
    }
#endif
#endif
}
