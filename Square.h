#ifndef SQUARE_H_
#define SQUARE_H_

#include <vector>

/*
    struct for representing a square in the grid.
*/
struct Square
{
    bool isVisible, wasVisible, isWater, isHill, isFood;
    int prevAnt, ant, hillPlayer;
    int lastSeenTurn;
    int timesVisible;
    int stationary;
    //std::vector<int> deadAnts;

    Square()
    {
        isVisible = wasVisible = isWater = isHill = isFood = 0;
        prevAnt = ant = hillPlayer = -1;
        lastSeenTurn = -1000000;
        stationary = 0;
    };

    //resets the information for the square except water information
    void reset()
    {
        if (ant > 0 && prevAnt == ant)
            ++stationary;
        else
            stationary = 0;
        prevAnt = ant;
        isVisible = 0;
        isHill = 0;
        isFood = 0;
        ant = hillPlayer = -1;
        //deadAnts.clear();
    }

    void setVisible(int turn)
    {
        isVisible = 1;
        wasVisible = 1;
        ++timesVisible;
        lastSeenTurn = turn;
    }

    void putDeadAnt(int who)
    {
        //deadAnts.push_back(who);
    }


};

#endif //SQUARE_H_
