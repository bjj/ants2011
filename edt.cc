#include "edt.h"

#include <iostream>
#include <iomanip>

#include <stdio.h>

Edt::Edt(State &_state)
    : Grid<int>(_state)
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

    if (empty_) {
        for (int r = 0; r < state.rows; ++r)
            for (int c = 0; c < state.cols; ++c)
                if ((*this)(r, c) == 0)
                    (*this)(r, c) = 9998;
        return;
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
