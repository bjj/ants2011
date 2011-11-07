#ifndef GRID_H_
#define GRID_H_

#include <string.h>

#include "Location.h"

class State;

extern int getStateRows(const State &state);
extern int getStateCols(const State &state);

template <typename T>
class Grid
{
public:
    Grid() : rows(0), cols(0), data(0) { }
    ~Grid() { delete data; }

    void init(const State &state) {
        if (!data) {
            rows = getStateRows(state);
            cols = getStateCols(state);
            data = new T[rows * cols];
        }
    }

    void reset()
    {
        // only appropriate for some things...
        memset(data, 0, rows * cols * sizeof(T));
    }

    T & operator () (const Location &loc)
    {
        return data[loc.row * cols + loc.col];
    }
    T & operator () (int r, int c)
    {
        return data[r * cols + c];
    }
    const T & operator () (const Location &loc) const
    {
        return data[loc.row * cols + loc.col];
    }
    const T & operator () (int r, int c) const
    {
        return data[r * cols + c];
    }
    T * operator [] (const int row)
    {
        return data + row * cols;
    }
    const T * operator [] (const int row) const
    {
        return data + row * cols;
    }

protected:

    int rows, cols;
    T *data;
};

#endif /* GRID_H_ */
