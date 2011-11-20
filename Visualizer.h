#ifndef _VISUALIZER_H
#define _VISUALIZER_H

#include <iostream>
#include <string>

#include "Location.h"

class Visualizer {
public:
    void setLineWidth(float width) const
    {
#ifdef VISUALIZER
        std::cout << "v setLineWidth " << width << std::endl;
#endif
    }
    void setLineColor(int r, int g, int b, float a = 1.0) const
    {
#ifdef VISUALIZER
        std::cout << "v setLineColor " << r << " " << g << " " << b << " " << a << std::endl;
#endif
    }
    void setFillColor(int r, int g, int b, float a = 0.5) const
    {
#ifdef VISUALIZER
        std::cout << "v setFillColor " << r << " " << g << " " << b << " " << a << std::endl;
#endif
    }
    void arrow(const Location &a, const Location &b) const
    {
#ifdef VISUALIZER
        std::cout << "v arrow " << a.row << " " << a.col << " " << b.row << " " << b.col << std::endl;
#endif
    }
    void line(const Location &a, const Location &b) const
    {
#ifdef VISUALIZER
        std::cout << "v line " << a.row << " " << a.col << " " << b.row << " " << b.col << std::endl;
#endif
    }
    void rect(const Location &a, const Location &b, bool fill) const
    {
#ifdef VISUALIZER
        std::cout << "v rect " << a.row << " " << a.col << " " << b.row << " " << b.col << " " << fill << std::endl;
#endif
    }
    void circle(const Location &a, float radius, bool fill) const
    {
#ifdef VISUALIZER
        std::cout << "v circle " << a.row << " " << a.col << " " << radius << " " << fill << std::endl;
#endif
    }
    void star(const Location &a, float r1, float r2, int points, bool fill) const
    {
#ifdef VISUALIZER
        std::cout << "v star " << a.row << " " << a.col << " " << r1 << " " << r2 << " " << points << " " << fill << std::endl;
#endif
    }
    void tile(const Location &a) const
    {
#ifdef VISUALIZER
        std::cout << "v tile " << a.row << " " << a.col << std::endl;
#endif
    }
    // TL, TM, TR, ML, MM, MR, BL, BM, BR
    void tileBorder(const Location &a, const char *subtile) const
    {
#ifdef VISUALIZER
        std::cout << "v tileBorder " << a.row << " " << a.col << " " << subtile << std::endl;
#endif
    }
    void tileSubTile(const Location &a, const char *subtile) const
    {
#ifdef VISUALIZER
        std::cout << "v tileSubTile " << a.row << " " << a.col << " " << subtile << std::endl;
#endif
    }
    void info(const Location &a, const std::string &info) const
    {
#ifdef VISUALIZER
        std::cout << "i " << a.row << " " << a.col << " " << info << std::endl;
#endif
    }
};

#endif /* _VISUALIZER_H */
