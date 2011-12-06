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
#include "Visualizer.h"
#include "Square.h"
#include "Location.h"
#include "grid.h"

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

#define PURE __attribute__((__pure__))

template <size_t N>
class Mod
{
public:
    Mod() : modulus(a + N / 2) { }
    void init(int n)
    {
        for (int i = -int(N) / 2; i < int(N) / 2; ++i)
            modulus[i] = (i + 10 * n) % n;
    }
    inline int operator () (const int &i) const PURE
    {
        return modulus[i];
    }
protected:
    int16_t *modulus;
    int16_t a[N]; __attribute__((aligned(64)));
};


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
        noPlayers, noHills;
    double attackradius, spawnradius, viewradius;
    double avgHillSpacing;
    double loadtime, turntime;
    std::vector<double> scores;
    bool gameover;
    int64_t seed;

    Grid<Square> grid;
    std::vector<Location> myAnts, enemyAnts, myHills, enemyHills, food;
    LocationSet allMyHills, allEnemyHills, allFood;
    std::vector<Location> visionNeighborhood, combatNeighborhood;
    int visibleSquares;
    typedef std::vector<Location>::iterator iterator;

    Mod<512> _row, _col;

    Timer timer;
    mutable Bug bug;
    mutable Visualizer v;

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
    Location getLocation(const Location &loc, int direction) const PURE
    {
        return Location(_row(loc.row + DIRECTIONS[direction][0]),
                        _col(loc.col + DIRECTIONS[direction][1]));
    }

    //returns the new location from moving in a given direction with the edges wrapped
    Location normLocation(const Location &loc) const PURE
    {
        return Location(_row(loc.row), _col(loc.col));
    }

    //returns the new location from moving in a given direction with the edges wrapped
    Location deltaLocation(const Location &loc, int dr, int dc) const PURE
    {
        return Location(_row(loc.row + dr), _col(loc.col + dc));
    }

    void updateVisionInformation();
    std::vector<Location> neighborhood_offsets(double max_dist) const;
    std::vector<Location> dialate_neighborhood(const std::vector<Location> &orig, int n) const;

};

extern State state;

struct Passable
{
    bool operator () (const Location &loc)
    {
        const Square &square = state.grid(loc);
        return !square.isWater;
    }
};

struct PassableButMyHills
{
    bool operator () (const Location &loc)
    {
        const Square &square = state.grid(loc);
        return !(square.isWater || square.hillPlayer == 0);
    }
};

std::ostream& operator<<(std::ostream &os, const State &state);
std::istream& operator>>(std::istream &is, State &state);

#endif //STATE_H_
