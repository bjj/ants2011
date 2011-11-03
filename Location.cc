#include "Location.h"

#include <iostream>
#include <iomanip>

using namespace std;

std::ostream& operator<<(std::ostream &os, const Location &loc)
{
    os << dec << "(" << loc.row << "," << loc.col << ")";
    return os;
}
