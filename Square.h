#ifndef SQUARE_H_
#define SQUARE_H_

#include <vector>

/*
    struct for representing a square in the grid.
*/
struct Square
{
    bool isVisible, wasVisible, isWater, isHill, isFood;
    bool frontier;
    int ant, hillPlayer;
    std::vector<int> deadAnts;

    Square()
    {
        isVisible = wasVisible = isWater = isHill = isFood = 0;
        ant = hillPlayer = -1;
    };

    //resets the information for the square except water information
    void reset()
    {
        frontier = 0;
        isVisible = 0;
        isHill = 0;
        isFood = 0;
        ant = hillPlayer = -1;
        deadAnts.clear();
    }

    void setVisible()
    {
        isVisible = 1;
        wasVisible = 1;
    }


};

#endif //SQUARE_H_
