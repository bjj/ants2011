#include "Bot.h"
#include "gridbfs.h"

using namespace std;

void Bot::maybeInitial()
{
    maybeEnemies.fill(true);

    GridBfs<Passable> end;

    for (State::iterator it = state.myHills.begin(); it != state.myHills.end(); ++it) {
        GridBfs<Passable> bfs(*it);

        for (; bfs.distance() < 20; ++bfs) { // game spec
            maybeEnemies(*bfs) = false;
        }
    }
}

void Bot::maybe()
{
    Grid<bool> expanded;

    for (int r = 0; r < state.rows; ++r) {
        bool *above = maybeEnemies[state._row(r - 1)];
        bool *below = maybeEnemies[state._row(r + 1)];
        bool *row = maybeEnemies[r];
        bool prevSeen = row[state.cols-1];
        bool nextSeen = row[0];
        for (int c = 1; c <= state.cols; ++c) {
            bool seen = nextSeen;
            nextSeen = row[state._col(c)];
            const Square &square = state.grid[r][c-1];
            if (square.isWater)
                expanded[r][c-1] = false;
            else if (square.isVisible)
                expanded[r][c-1] = square.ant > 0;
            else {
                bool aboveSeen = above[c-1];
                bool belowSeen = below[c-1];
                expanded[r][c-1] = prevSeen | seen | nextSeen | aboveSeen | belowSeen;
            }
            prevSeen = seen;
        }
    }

    maybeEnemies.swap(expanded);

#ifdef VISUALIZER
#if 0
    state.v.setLineColor(128,0,0,0.5);
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            if (maybeEnemies[r][c])
                state.v.tileBorder(Location(r,c), "MM");
        }
    }
    state.v.setLineColor(0,0,255,0.5);
#endif
#endif
}
