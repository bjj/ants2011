#include "Bot.h"

using namespace std;

void Bot::pushy()
{
    e_push.update(state.enemyAnts.begin(), state.enemyAnts.end());

    vector<Location> intercepts;
    for (int row = 0; row < state.rows; ++row) {
        for (int col = 0; col < state.cols; ++col) {
            if (!state.grid[row][col].isWater && e_push[row][col] + 3 == e_self[row][col])
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
#if 0
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
