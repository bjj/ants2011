#ifndef GRIDBFS_H_
#define GRIDBFS_H_

#include "grid.h"

#include <queue>
#include <iterator>

template <typename Passable>
class GridBfs : std::iterator<std::forward_iterator_tag, Location>
{
public:
    GridBfs()
        : dist(-1)
    { }

    template <typename T>
    GridBfs(const Grid<T> &grid, Passable &p, const Location &start)
        : dist(0)
        , passable(p)
    {
        visited.init(grid);
        q.push(start);
        visited(start) = true;
        q.push(Location(-1, -1));
        enqueueAll();
    }

    GridBfs(const GridBfs &other)
        : dist(other.dist)
        , passable(other.passable)
        , q(other.q)
        , visited(other.visited)
    { }


    const Location & operator * () const
    {
        return q.front();
    }

    const Location * operator -> () const
    {
        return &(operator*());
    }

    GridBfs & operator ++ ()
    {
        q.pop();
        while (q.front().row == -1) {
            dist++;
            q.pop();
            if (!q.empty())
                q.push(Location(-1, -1));
        }
        if (q.empty())
            dist = -1;
        else
            enqueueAll();
        return *this;
    }

    GridBfs operator ++ (int)
    {
        GridBfs tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator == (const GridBfs &rhs) const
    {
        // XXX poor condition
        return dist == rhs.dist && (dist == -1 || q == rhs.q);
    }

    bool operator != (const GridBfs &rhs) const
    {
        return !(*this == rhs);
    }

    int distance() const
    {
        return dist;
    }

protected:
    void enqueue(int dr, int dc)
    {
        int r = (q.front().row + dr + visited.rows) % visited.rows;
        int c = (q.front().col + dc + visited.cols) % visited.cols;
        const Location loc(r, c);
        if (!visited(loc) && passable(loc)) {
            q.push(loc);
            visited(loc) = true;
        }
    }
    void enqueueAll()
    {
        const Location &here = q.front();
        enqueue(-1, 0);
        enqueue(0, 1);
        enqueue(1, 0);
        enqueue(0, -1);
    }
    int dist;
    Passable passable;
    std::queue<Location> q;
    Grid<bool> visited;
};

#endif /* GRIDBFS_H_ */