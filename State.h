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
    std::vector<int> diedByPlayer, ateByPlayer;
    std::vector<Location> myAnts, enemyAnts, myHills, enemyHills, food;
    LocationSet allMyHills, allEnemyHills, allFood;
    std::vector<Location> visionNeighborhood, combatNeighborhood;
    std::vector<std::vector<Location> > visionArc;
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

    int shortRowDiff(int r1, int r2) const PURE
    {
        int d = r1 - r2;
        if (d > rows / 2)
            d -= rows;
        return d;
    }

    int shortColDiff(int c1, int c2) const PURE
    {
        int d = c1 - c2;
        if (d > cols / 2)
            d -= cols;
        return d;
    }

    void updateVisionInformation();
    std::vector<Location> neighborhood_offsets(double max_dist) const;
    std::vector<Location> dialate_neighborhood(const std::vector<Location> &orig, int n) const;
    std::vector<Location> neighborhood_arc(double max_dist, int direction) const;

};

extern State state;

struct AllPassable
{
    bool operator () (const Location &loc) const
    {
        return true;
    }
};

struct Passable
{
    bool operator () (const Location &loc) const
    {
        const Square &square = state.grid(loc);
        return !square.isWater;
    }
};

struct PassableButMyHills
{
    bool operator () (const Location &loc) const
    {
        const Square &square = state.grid(loc);
        return !(square.isWater || square.hillPlayer == 0);
    }
};

template <typename P = Passable>
struct Unidirectional : public P
{
    Unidirectional(const Grid<int> &f) : flow(f) { }


    bool operator () (const Location &loc) const
    {
        return P::operator()(loc) && flow(loc) <= origin;
    }

    void setOrigin(const Location &loc)
    {
        origin = flow(loc);
    }

private:
    const Grid<int> &flow;
    int origin;
};

struct UnitCost
{
    inline bool operator () (const Location &loc) const
    {
        return 1;
    }
};

struct NarrowCost
{
    inline bool operator () (const Location &loc) const
    {
        //                   OPEN WALL GAP/CORNER NICHE STUCK
        const int costs[] = { 1,   2,     20,      50,  1000 };
        const Square &square = state.grid(loc);
        return costs[int(square.byWater)];
    }
};

struct NarrowBusyCost : NarrowCost
{
    inline bool operator () (const Location &loc) const
    {
        const Square &square = state.grid(loc);
        return NarrowCost::operator()(loc) + 4 * square.stationary;
    }
};

std::ostream& operator<<(std::ostream &os, const State &state);
std::istream& operator>>(std::istream &is, State &state);

#include <iostream>
#include <iomanip>

template <typename T>
std::ostream& operator<<(std::ostream &os, const Grid<T> &grid)
{
    os << std::hex;
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            if (grid(r,c) == 9999)
                os << " []";
            else if (grid(r,c) >= 1000)
                os << " __";
            else if (grid(r,c) > 0xff)
                os << " xx";
            else
                os << std::setw(3) << grid(r,c);
        }
        os << std::endl;
    }
    os << std::dec;
    return os;
}

template <typename I, typename J>
void paint(Grid<bool> &grid, I begin, I end, J nbegin, J nend)
{
    for (; begin != end; ++begin)
        for (J n = nbegin; n != nend; ++n)
            grid(state.deltaLocation(*begin, (*n).row, (*n).col)) = true;
}


#endif //STATE_H_
