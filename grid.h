#ifndef GRID_H_
#define GRID_H_

#include <algorithm>
#include <string.h>

#include "Location.h"

class State;
template <typename Passable> class GridBfs;

extern int getStateRows(const State &state);
extern int getStateCols(const State &state);

template <typename T>
class Grid
{
public:
    Grid() : rows(0), cols(0), data(0) { }
    Grid(const Grid &other)
        : rows(other.rows), cols(other.cols), data(0)
    {
        data = new T[rows * cols];
        int end = rows * cols;
        std::copy(other.data, other.data + rows * cols, data);
    }
    ~Grid() { delete data; }

    template <typename U>
    void init(const Grid<U> &grid)
    {
        init(grid.rows, grid.cols);
    }
    void init(const State &state)
    {
        init(getStateRows(state), getStateCols(state));
    }
    void init(int r, int c)
    {
        if (!data) {
            rows = r;
            cols = c;
            data = new T[rows * cols];
        }
    }

    void reset()
    {
        std::fill(data, data + rows * cols, T());
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
    Grid & operator = (const Grid &rhs)
    {
        rows = rhs.rows;
        cols = rhs.cols;
        delete data;
        data = new T[rows * cols];
        int end = rows * cols;
        for (int i = 0; i < end; ++i)
            data[i] =rhs.data[i];
    }
    ~Grid() { delete data; }

protected:
    template <typename> friend class GridBfs;
    template <typename> friend class Grid;

    int rows, cols;
    T *data;
};

#endif /* GRID_H_ */
