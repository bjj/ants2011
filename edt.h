#ifndef EDT_H_
#define EDT_H_

#include <string.h>
#include <queue>
#include <algorithm>

#include "State.h"


template <typename T>
class Grid
{
public:
    Grid(State &_state) : state(_state), data(0) { }

    void reset()
    {
        if (data == 0)
            data = new T[state.rows * state.cols];
        memset(data, 0, state.rows * state.cols * sizeof(T));
    }
    T & operator () (const Location &loc)
    {
        return data[loc.row * state.cols + loc.col];
    }
    T & operator () (int r, int c)
    {
        return data[r * state.cols + c];
    }
    const T & operator () (const Location &loc) const
    {
        return data[loc.row * state.cols + loc.col];
    }
    const T & operator () (int r, int c) const
    {
        return data[r * state.cols + c];
    }

protected:
    const State &state;
    T *data;
};

class Edt : public Grid<int>
{
public:
    Edt(State &_state);
    void update(std::queue<Location> &food);
    int gradient(const Location &loc) const
    {
        if (empty)
            return -1;
        int best_dist = (*this)(loc);
        int best = -1;
        for (int d = 0; d < TDIRECTIONS; ++d) {
            Location targ = state.getLocation(loc, d);
            if ((*this)(targ) < best_dist) {
                best_dist = (*this)(targ);
                best = d;
            }
        }
        return best;
    }

protected:
    void enqueue(std::queue<Location> & q, int r, int c);

    bool empty;
};

#endif /* EDT_H_ */
