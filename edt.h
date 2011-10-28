#ifndef EDT_H_
#define EDT_H_

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

protected:
    const State &state;
    T *data;
};

class Edt
{
public:
    Edt(State &_state);
    void update(std::queue<Location> &food);
    int & operator () (const Location &loc)
    {
        return dist(loc);
    }
    int & operator () (int r, int c)
    {
        return dist(r, c);
    }

protected:
    void enqueue(std::queue<Location> & q, int r, int c);
    State &state;
    Grid<int> dist;
};

#endif /* EDT_H_ */
