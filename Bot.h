#ifndef BOT_H_
#define BOT_H_

#include "State.h"
#include "edt.h"

struct Move
{
    Move(const Location &l, int d, int c)
        : loc(l), dir(d), close(c) { }


    bool operator < (const Move &rhs) const
    {
        if ((rhs.dir == -1) == (dir == -1))
            return close > rhs.close;
        else
            return dir == -1;
    }

    Location loc;
    int dir, close;
};

/*
    This struct represents your bot in the game of Ants
*/
struct Bot
{
    State state;
    Edt e_food, e_explore, e_attack, e_defend, e_enemies;
    Grid<bool> busy;
    std::vector<Location> interesting;

    Bot();

    void playGame();    //plays a single game of Ants

    void makeMoves();   //makes moves for a single turn
    void endTurn();     //indicates to the engine that it has made its moves
    std::queue<Location> visited_frontier();
    Move pickMove(const Location &loc) const;

};

#endif //BOT_H_
