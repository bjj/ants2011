#ifndef EDT_H_
#define EDT_H_

#include <string.h>
#include <queue>
#include <string>
#include <algorithm>
#include <iostream>
#include <iterator>

#include "State.h"
#include "grid.h"


class Edt;

std::ostream& operator<<(std::ostream &os, const Edt &edt);

class Edt : public Grid<int>
{
public:
    Edt(std::string _name, State &_state);
    void update(std::queue<Location> &food);
    template <typename I>
    void update(I begin, I end)
    {
        std::queue<Location> food;
        for (; begin != end; ++begin)
            food.push(*begin);
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
    bool empty() const { return empty_; }
    double euclidean(const Location &loc, int limit = 9999) const;

    std::string name;

protected:
    friend std::ostream& operator<<(std::ostream &os, const Edt &edt);

    void enqueue(std::queue<Location> & q, int r, int c);

    bool empty_;
};

#endif /* EDT_H_ */
