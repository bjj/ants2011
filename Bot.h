#ifndef BOT_H_
#define BOT_H_

#include "State.h"
#include "edt.h"

#include <queue>
#include <vector>

struct Move
{
    Move(const Location &l, int d, int s, int c, const std::string &w)
        : loc(l), dir(d), score(s), close(c), why(&w) { }
    Move(const Move &other)
        : loc(other.loc)
        , dir(other.dir)
        , score(other.score)
        , close(other.close)
        , why(other.why)
    { }


    struct BestScore
    {
        bool operator () (const Move &lhs, const Move &rhs) const
        {
            if ((rhs.dir == -1) == (lhs.dir == -1))
                return lhs.score > rhs.score;
            else
                return lhs.dir == -1;
        }
    };

    struct Closest
    {
        bool operator () (const Move &lhs, const Move &rhs) const
        {
            if ((rhs.dir == -1) == (lhs.dir == -1))
                return lhs.close > rhs.close;
            else
                return lhs.dir == -1;
        }
    };

    typedef std::priority_queue<Move, std::vector<Move>, BestScore> score_queue;
    typedef std::priority_queue<Move, std::vector<Move>, Closest> close_queue;

    Location loc;
    int dir, score, close;
    const std::string *why;
};

/*
    This struct represents your bot in the game of Ants
*/
struct Bot
{
    State state;
    Edt e_food, e_explore, e_revisit, e_attack, e_defend, e_enemies, e_self, e_myHills;
    Grid<char> busy;
    Grid<bool> combatOccupied;
    std::set<Location> interesting;

    Bot();

    void playGame();    //plays a single game of Ants

    void makeMoves();   //makes moves for a single turn
    void endTurn();     //indicates to the engine that it has made its moves
    template <typename Predicate>
    std::vector<Location> frontier();
    Move pickMove(const Location &loc) const;
    void visualize();

    void combat(Move::close_queue &moves, std::set<Location> &sessile);
    void eat(Move::close_queue &moves, std::set<Location> &sessile);

};

#endif //BOT_H_
