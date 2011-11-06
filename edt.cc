#include "edt.h"

#include <iostream>
#include <iomanip>

#include <stdio.h>

Edt::Edt(std::string _name, State &_state)
    : Grid<int>(_state)
    , name(_name)
{
    ;
}

inline void
Edt::enqueue(std::queue<Location> & q, int r, int c)
{
    r = (r + state.rows) % state.rows;
    c = (c + state.cols) % state.cols;
    if ((*this)(r, c) == 0)
        q.push(Location(r, c));
}

void
Edt::update(std::queue<Location> &food)
{
    reset();

    empty_ = food.empty();

    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            if (state.grid[r][c].isWater || state.grid[r][c].hillPlayer == 0)
                (*this)(r, c) = 9999;
        }
    }

    const Location marker(-1, -1);
    food.push(marker);
    int d = 1;
    while (!food.empty()) {
        const Location &loc = food.front();
        const int r = loc.row;
        const int c = loc.col;
        food.pop();
        if (r == -1) {
            if (!food.empty())
                food.push(marker);
            d++;
            continue;
        }
        if ((*this)(r, c) == 0) {
            (*this)(r, c) = d;
            enqueue(food, r+1, c);
            enqueue(food, r, c+1);
            enqueue(food, r-1, c);
            enqueue(food, r, c-1);
        }
    }

    // Anything unreachable
    for (int r = 0; r < state.rows; ++r)
        for (int c = 0; c < state.cols; ++c)
            if ((*this)(r, c) == 0)
                (*this)(r, c) = 9998;
}

/**
 * Our grid contains the Manhattan distance to any target.  To convert to
 * Euclidean distance we walk back to the source and then do the math.
 */
double
Edt::euclidean(const Location &loc, int limit) const
{
    Location walk = loc;
    int close = 0, dir = 0;

    do {
        dir = gradient(walk, &close);
        if (dir != -1)
            walk = state.getLocation(walk, dir);
        if (close > limit)
            return 9999;
    } while (dir != -1 && close != 1);

    if (close == 1)
        return state.distance(loc, walk);
    else
        return 9999;
}


std::ostream& operator<<(std::ostream &os, const Edt &edt)
{
    os << std::hex;
    for (int r = 0; r < edt.state.rows; ++r) {
        for (int c = 0; c < edt.state.cols; ++c) {
            if (edt(r,c) >= 1000)
                os << " __";
            else if (edt(r,c) > 0xff)
                os << " xx";
            else
                os << std::setw(3) << edt(r,c);
        }
        os << std::endl;
    }
    os << std::dec;
    return os;
}

