#ifndef STATE_H_
#define STATE_H_

#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <stdint.h>

#include "Timer.h"
#include "Bug.h"
#include "Square.h"
#include "Location.h"

/*
    constants
*/
const int TDIRECTIONS = 4;
const char CDIRECTIONS[5] = {'N', 'E', 'S', 'W', 'x'};
const int DIRECTIONS[5][2] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1}, {0, 0} };      //{N, E, S, W}
const int AHEAD[5] = { 0, 1, 2, 3, 4 };
const int BEHIND[5] = { 2, 3, 0, 1, 4 };
const int RIGHT[5] = { 1, 2, 3, 0, 4 };
const int LEFT[5] = { 3, 0, 1, 2, 4 };

/*
    struct to store current state information
*/
struct State
{
    /*
        Variables
    */
    int rows, cols,
        turn, turns,
        noPlayers;
    double attackradius, spawnradius, viewradius;
    double loadtime, turntime;
    std::vector<double> scores;
    bool gameover;
    int64_t seed;

    std::vector<std::vector<Square> > grid;
    std::vector<Location> myAnts, enemyAnts, myHills, enemyHills, food;
    std::set<Location> allEnemyHills, allFood;
    typedef std::vector<Location>::iterator iterator;

    Timer timer;
    mutable Bug bug;

    /*
        Functions
    */
    State();
    ~State();

    void setup();
    void reset();

    void makeMove(const Location &loc, int direction);

    double distance(const Location &loc1, const Location &loc2) const;

    //returns the new location from moving in a given direction with the edges wrapped
    Location getLocation(const Location &loc, int direction) const
    {
        return Location( (loc.row + DIRECTIONS[direction][0] + rows) % rows,
                         (loc.col + DIRECTIONS[direction][1] + cols) % cols );
    }

    //returns the new location from moving in a given direction with the edges wrapped
    Location normLocation(const Location &loc) const
    {
        return Location( (loc.row + rows) % rows,
                         (loc.col + cols) % cols );
    }

    //returns the new location from moving in a given direction with the edges wrapped
    Location deltaLocation(const Location &loc, int dr, int dc) const
    {
        return Location( (loc.row + dr + rows) % rows,
                         (loc.col + dc + cols) % cols );
    }

    void updateVisionInformation();
};

std::ostream& operator<<(std::ostream &os, const State &state);
std::istream& operator>>(std::istream &is, State &state);

#endif //STATE_H_
