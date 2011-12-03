#ifndef LOCATION_H_
#define LOCATION_H_

#include <iostream>

enum { LocInf = 999999 };

/*
    struct for representing locations in the grid.
*/
struct Location
{
    int row, col;

    Location()
        :row(0), col(0)
    { }

    Location(int r, int c)
        :row(r), col(c)
    { }

    Location(const Location &other)
        :row(other.row), col(other.col)
    { }

    bool operator < (const Location &other) const
    {
        return (row == other.row ? (col < other.col) : (row < other.row));
    }

    bool operator == (const Location &other) const
    {
        return row == other.row && col == other.col;
    }
};

struct LocationHash {
    inline size_t operator () (const Location &loc) const
    {
        return loc.col + loc.row * 256;
    }
};

#if 0
#include <tr1/unordered_set>
typedef std::tr1::unordered_set<Location, LocationHash> LocationSet;
#else
#include <set>
typedef std::set<Location> LocationSet;
#endif

std::ostream& operator<<(std::ostream &os, const Location &loc);


#endif //LOCATION_H_
