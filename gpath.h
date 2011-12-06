#ifndef GPATH_H_
#define GPATH_H_

#include <string.h>
#include <queue>
#include <string>
#include <algorithm>
#include <iostream>
#include <iterator>

#include "enable_if.h"

#include "grid.h"

template <typename Passable, typename Cost>
class Gpath : public Grid<int>
{
protected:
    struct QElem {
        QElem(const Location &l, int d)
            : loc(l), dist(d) { }
        QElem(int r, int c, int d)
            : loc(Location(r,c)), dist(d) { }
        Location loc;
        int dist;
    };

public:
    Gpath()
        : empty_(true)
    { }
    Gpath(std::string _name)
        : name(_name)
        , empty_(true)
    { }
    Gpath(std::string _name, Passable p)
        : name(_name)
        , passable(p)
        , empty_(true)
    { }
    Gpath(std::string _name, Cost c)
        : name(_name)
        , cost(c)
        , empty_(true)
    { }
    Gpath(std::string _name, Passable p, Cost c)
        : name(_name)
        , passable(p)
        , cost(c)
        , empty_(true)
    { }

    template <typename I>
    void update(I begin, I end)
    {
        fill(9999);
        std::queue<QElem> food;
        for (; begin != end; ++begin)
            enqueue(food, 0, *begin);
        update(food);
    }

    int gradient(const Location &loc, int *close = 0) const
    {
        if (empty_)
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
        if (close)
            *close = best_dist;
        return best;
    }

    bool toward(const Location &loc, const Location &dest) const
    {
        return (*this)(dest) < (*this)(loc);
    }

    bool empty() const { return empty_; }

protected:
    inline void enqueue(std::queue<QElem> & q, int d, const Location &loc)
    {
        d += cost(loc);
        if ((*this)(loc) > d) {
            (*this)(loc) = d;
            q.push(QElem(loc, d));
        }
    }

    void enqueue(std::queue<QElem> & q, int d, const Location &loc, int dr, int dc)
    {
        const Location dest = state.deltaLocation(loc, dr, dc);
        if (passable(dest))
            enqueue(q, d, dest);
    }

    template <typename V>
    struct has_member_setOrigin
    {
        template <typename U, void (U::*)(const Location &)> struct SFINAE {};
        template <typename U> static char test(SFINAE<U, &U::setOrigin>*);
        template <typename U> static int test(...);
        static const bool value = sizeof(test<V>(0)) == sizeof(char);
    };

    template<typename V>
    inline __attribute__((__always_inline__))
    void callSetOrigin(typename enable_if <has_member_setOrigin<V>::value, V>::type &p, const Location &loc) const
    {
        p.setOrigin(loc);
    }

    template<typename V>
    inline __attribute__((__always_inline__))
    void callSetOrigin(typename enable_if <!has_member_setOrigin<V>::value, V>::type &p, const Location &loc) const
    {
    }

    inline void enqueueAll(std::queue<QElem> & q, int d, const Location &loc) __attribute__((__flatten__))
    {
        callSetOrigin<Passable>(passable, loc);
        enqueue(q, d, loc, +1, 0);
        enqueue(q, d, loc, 0, +1);
        enqueue(q, d, loc, -1, 0);
        enqueue(q, d, loc, 0, -1);
    }

    void update(std::queue<QElem> &food) __attribute__((__flatten__))
    {
        empty_ = food.empty();

        while (!food.empty()) {
            const QElem &qe = food.front();
            enqueueAll(food, qe.dist, qe.loc);
            food.pop();
        }
    }

public:
    std::string name;
protected:
    Passable passable;
    Cost cost;
    bool empty_;
};

#endif
