#ifndef GRID_H_
#define GRID_H_

#include <algorithm>
#include <string.h>

#include "Location.h"

template <typename Passable> class GridBfs;

struct GridBase {
    static int rows, cols;
};

template <typename T>
class Grid : public GridBase
{
public:
    Grid()
        : data(0)
    {
        data = new T[rows * cols];
    }
    Grid(const Grid &other)
        : data(0)
    {
        data = new T[rows * cols];
        std::copy(other.data, other.data + rows * cols, data);
    }
    ~Grid()
    {
        delete data;
    }

    void fill(const T &value)
    {
        std::fill(data, data + rows * cols, value);
    }

    void reset()
    {
        fill(T());
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
        copy(rhs.data, rhs.data + rows * cols, data);
        return *this;
    }

    void swap(Grid &other)
    {
        std::swap(data, other.data);
    }

protected:
    template <typename> friend class GridBfs;
    template <typename> friend class Grid;

    T *data;
};

struct DistanceTag
{
    DistanceTag(const Grid<int> &g) : grid(g) { }
    std::pair<int, Location> operator () (const Location &loc) const
    {
        return std::make_pair(grid(loc), loc);
    }
protected:
    const Grid<int> &grid;
};

#endif /* GRID_H_ */
