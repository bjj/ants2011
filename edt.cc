#include "edt.h"

#include <stdio.h>

Edt::Edt(State &_state)
    : state(_state)
    , dist(_state)
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
    dist.reset();
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            if (state.grid[r][c].isWater)
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

#if 0
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            int & val = (*this)(r, c);
            if (val == 9999)
                fprintf(stderr, "** ");
            else
                fprintf(stderr, "%2d ", val);
        }
        fputc('\n', stderr);
    }
#endif
}
