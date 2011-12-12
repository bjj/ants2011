#include <stdlib.h>
#include <math.h>

#include "Bot.h"
#include "edt.h"

using namespace std;

//makes the bots moves for the turn
void Bot::visualize()
{
#ifdef VISUALIZER
    LocationSet foodSet(state.food.begin(), state.food.end());
    // hilite known but unseen food:
    vector<Location> unseen;
    set_difference(state.allFood.begin(), state.allFood.end(), foodSet.begin(), foodSet.end(), back_inserter(unseen));
    LocationSet hillSet(state.enemyHills.begin(), state.enemyHills.end());
    set_difference(state.allEnemyHills.begin(), state.allEnemyHills.end(), hillSet.begin(), hillSet.end(), back_inserter(unseen));
    state.v.setLineColor(255, 255, 0);
    for (uint i = 0; i < unseen.size(); ++i)
        state.v.star(unseen[i], 0.7, 0.8, 4, false);

#if 0
    const Grid<int> &EDT = e_frontline;
    state.v.setLineColor(255,255,255,0.5);
    for (int row = 0; row < state.rows; ++row)
        for (int col = 0; col < state.cols; ++col)
            if (EDT[row][col] == 1)
                state.v.tileBorder(Location(row,col), "MM");
    state.v.setLineColor(200,200,200,0.5);
    for (int row = 0; row < state.rows; ++row)
        for (int col = 0; col < state.cols; ++col)
            if (EDT[row][col] == 2)
                state.v.tileBorder(Location(row,col), "MM");
    state.v.setLineColor(150,150,150,0.5);
    for (int row = 0; row < state.rows; ++row)
        for (int col = 0; col < state.cols; ++col)
            if (EDT[row][col] == 3)
                state.v.tileBorder(Location(row,col), "MM");
#endif

#endif
}
