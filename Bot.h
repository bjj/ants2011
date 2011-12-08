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
    int maxVisibleSquares, maxVisibleTurn;
    Edt e_food;
    UniEdt e_explore;
    Edt e_revisit, e_attack, e_defend;
    Gpath<Passable, UnitCost> e_enemies, e_self, e_myHills;
    Grid<char> busy;
    Grid<bool> combatOccupied;
    Grid<bool> enemyThreat, selfThreat;;
    Grid<int> combatLabels;
    LocationSet interesting;
    std::vector<Location> hotspots;
    int myInitialAnts, myFoodEaten, myDeadAnts, myNewAntTurn;

    std::vector<Location> homeDefense, visionNeighborhood_m1;

    Bot();

    void playGame();    //plays a single game of Ants

    void makeMoves();   //makes moves for a single turn
    void endTurn();     //indicates to the engine that it has made its moves
    template <typename Predicate>
    std::vector<Location> frontier(const Predicate &pred);
    Move pickMove(const Location &loc) const;
    void visualize();

    void combat(Move::close_queue &moves, LocationSet &sessile);
    void territory(Move::close_queue &moves, LocationSet &sessile);
    void eat(Move::close_queue &moves, LocationSet &sessile);

    void combatLabel(std::vector<int> &equiv, int &nextLabel, const std::vector<Location> &ants);
    std::vector<Location> combatThreat(const std::vector<Location> &ants, const std::vector<Location> &enemies, Grid<bool>& threat);
    void combatGroup(Move::close_queue &moves, LocationSet &sessile, const std::vector<Location> &, const std::vector<Location> &);

    int hive() const;
    void resetHive();
    void updateHive();
};

#endif //BOT_H_
