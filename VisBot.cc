#include <stdlib.h>
#include <math.h>

#include "Bot.h"
#include "edt.h"

using namespace std;

//makes the bots moves for the turn
void Bot::visualize()
{
#ifdef VISUALIZER
    set<Location> foodSet(state.food.begin(), state.food.end());
    // hilite known but unseen food:
    vector<Location> unseen;
    set_difference(state.allFood.begin(), state.allFood.end(), foodSet.begin(), foodSet.end(), back_inserter(unseen));
    set<Location> hillSet(state.enemyHills.begin(), state.enemyHills.end());
    set_difference(state.allEnemyHills.begin(), state.allEnemyHills.end(), hillSet.begin(), hillSet.end(), back_inserter(unseen));
    state.v.setLineColor(255, 255, 0);
    for (uint i = 0; i < unseen.size(); ++i)
        state.v.star(unseen[i], 0.7, 0.8, 4, false);
#endif
}
