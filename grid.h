#ifndef GRID_H_
#define GRID_H_

#include <string.h>

class State;

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

#endif /* GRID_H_ */
