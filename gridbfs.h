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

    GridBfs(const Location &start, const Passable &p = Passable())
        : dist(0)
        , passable(p)
    {
        visited.reset();
        q.push(start);
        visited(start) = TDIRECTIONS + 1; // "don't move"
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

    int direction() const
    {
        return visited(q.front()) - 1;
    }

    int direction2() const
    {
        int d = direction();
        Location next = state.getLocation(q.front(), d);
        int d2 = visited(next) - 1;
        if (d2 != -1 && d2 != TDIRECTIONS) {
            next = state.getLocation(q.front(), d2);
            int d3 = visited(next) - 1;
            if (d3 != -1 && d3 != TDIRECTIONS) {
                if (BEHIND[d3] != d2)
                    return d2;
            }
        }
        return d;
    }

protected:
    void enqueue(int dr, int dc, int dir)
    {
        int r = state._row(q.front().row + dr);
        int c = state._col(q.front().col + dc);
        const Location loc(r, c);
        if (!visited(loc) && passable(loc)) {
            q.push(loc);
            visited(loc) = dir + 1;
        }
    }
    void enqueueAll()
    {
        enqueue(-1, 0, 2); // go back S
        enqueue(0, 1, 3);  // go back W
        enqueue(1, 0, 0);  // go back N
        enqueue(0, -1, 1); // go back E
    }
    int dist;
    Passable passable;
    std::queue<Location> q;
    Grid<char> visited;
};

#endif /* GRIDBFS_H_ */
