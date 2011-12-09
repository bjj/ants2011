#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "State.h"
#include "Bot.h"
#include "anneal.h"
#include "gridbfs.h"

using namespace std;

class Territory
{
public:
    struct Ant {
        Ant(const Location &l) : loc(l), dir(TDIRECTIONS), cost(250) { }
        struct Move {
            Move() : occupied(0), bonus(0) { }
            bool *occupied;
            int bonus;
            vector<Location> gain;
            vector<Location> lose;
        };
        Location loc;
        Move moves[TDIRECTIONS+1];
        int dir;
        int cost;

        inline int score() const
        {
            return moves[dir].bonus;
        }
    };

    Territory(State &s, vector<Ant> &a, Grid<int> &t)
        : state(s)
        , ants(a)
        , territory(t)
        , _score(0)
    {
        for (uint i = 0; i < ants.size(); ++i)
            apply(i);
    }

    Territory(const Territory &other)
        : state(other.state)
        , ants(other.ants)
        , territory(other.territory)
        , _score(other._score)
        , undo_i(other.undo_i)
        , undo_dir(other.undo_dir)
    {
    }

    ~Territory()
    {
    }

    double e() const
    {
        return -_score;
    }

    size_t iterations() const
    {
        return ants.size() * 5;
    }

    void neighbor()
    {
        int retry = 10;
        int dir;
        do {
            undo_i = random() % ants.size();
            undo_dir = ants[undo_i].dir;
            do {
                dir = random() % (TDIRECTIONS + 1);
            } while (undo_dir == dir);
        } while (!change(undo_i, dir) && --retry > 0);
    }

    void undo()
    {
        change(undo_i, undo_dir);
    }

    Territory & operator = (const Territory & rhs)
    {
        ants = rhs.ants;
        territory = rhs.territory;
        _score = rhs._score;
        undo_i = rhs.undo_i;
        undo_dir = rhs.undo_dir;
        return *this;
    }

private:

    Territory() : state(*((State *)0)) { }

    void apply(int n)
    {
        Territory::Ant &ant = ants[n];
        Territory::Ant::Move &move = ant.moves[ant.dir];
        *move.occupied = true;
        _score += ant.score();
    }

    bool change(int n, int dir)
    {
        Territory::Ant &ant = ants[n];
        if (dir == ant.dir)
            return false;
        Territory::Ant::Move &old = ant.moves[ant.dir];
        Territory::Ant::Move &move = ant.moves[dir];
        if (*move.occupied)
            return false;
        *old.occupied = false;
        *move.occupied = true;
        _score -= ant.score();
        ant.dir = dir;

        for (uint i = 0; i < old.gain.size(); ++i) {
            int &count = territory(old.gain[i]);
            if (--count == 0)
                --_score;
        }

        for (uint i = 0; i < move.lose.size(); ++i) {
            int &count = territory(move.lose[i]);
            if (--count == 0)
                --_score;
        }

        for (uint i = 0; i < old.lose.size(); ++i) {
            int &count = territory(old.lose[i]);
            if (++count == 1)
                ++_score;
        }

        for (uint i = 0; i < move.gain.size(); ++i) {
            int &count = territory(move.gain[i]);
            if (++count == 1)
                ++_score;
        }
        _score += ant.score();
        return true;
    }

    friend ostream & operator << (ostream &os, const Territory &territory);

    State &state;
public:
    vector<Ant> ants;
    Grid<int> territory;
private:
    int _score;
    int undo_i;
    char undo_dir;
};

ostream & operator << (ostream &os, const Territory &territory)
{
    os << "territory score: " << -territory.e() << endl;
    return os;
}

struct Offset {
    Offset(const Location &l) : loc(l) { }
    Location operator () (const Location &disp) const
    {
        return state.deltaLocation(loc, disp.row, disp.col);
    }
private:
    Location loc;
};

struct PassableAndCanUnsee : Passable {
    PassableAndCanUnsee(const Grid<bool> &cU) : cantUnsee(cU) { }
    bool operator () (const Location &loc) const
    {
        return Passable::operator()(loc) && !cantUnsee(loc);
    }
private:
    const Grid<bool> &cantUnsee;
};

// ok this predicate application is probably backwards from what
// would really be used in a std::transform_if...
template <typename I, typename O, typename UnaryFunction, typename Predicate>
O transform_if(I begin, I end, O out, UnaryFunction f, Predicate pred)
{
    for (; begin != end; ++begin) {
        typeof(*begin) tmp = f(*begin);
        if (pred(tmp))
            *out++ = tmp;
    }
    return out;
}

/**
 * Identify which ants are in position to be in territory and handle
 * them specially.
 */
void Bot::territory(Move::close_queue &moves, LocationSet &sessile)
{
    Grid<bool> cantUnsee;
    cantUnsee.reset();
    paint(cantUnsee, state.myAnts.begin(), state.myAnts.end(),
        visionNeighborhood_m1.begin(), visionNeighborhood_m1.end());

    Grid<int> tgrid;
    tgrid.reset();

    vector<Territory::Ant> ants;

    int attackBonusMul = 23;
    int exploreBonusMul = 17;
    int minExplore = maxVisibleSquares * 85 / 100;
    int targetExplore = minExplore + (state.turn - maxVisibleTurn) * (state.viewradius * 2) * 4;
    targetExplore = min(targetExplore, state.rows * state.cols * 95 / 100);
    if (state.visibleSquares < minExplore)
        exploreBonusMul = attackBonusMul + 5;
    if (state.visibleSquares < targetExplore)
        exploreBonusMul = attackBonusMul - 1;

#if 0
#ifdef VISUALIZER
    stringstream exploreInfo;
    exploreInfo << "vis " << state.visibleSquares << " min " << minExplore << " targ " << targetExplore;
    state.v.info(Location(0,0), exploreInfo.str());
#endif
#endif

    for (State::iterator it = state.myAnts.begin(); it != state.myAnts.end(); ++it) {
        if (sessile.count(*it))
            continue;
        ants.push_back(Territory::Ant(*it));
        Territory::Ant &ant = ants.back();

        int bonusMul = 2 + 3 * (e_enemies(*it) < 5 || e_explore(*it) < 15);
        for (int d = 0; d < TDIRECTIONS + 1; ++d) {
            const Location dest = state.getLocation(*it, d);
            ant.moves[d].occupied = &combatOccupied(dest);
            int bonus = 0;
            if (state.grid(dest).hillPlayer == 0)
                bonus = -100;

            if (e_explore.toward(*it, dest))
                bonus += 1;
            if (e_intercept(*it) < 10 && e_intercept.toward(*it, dest))
                bonus += 5;
            if (e_attack.toward(*it, dest))
                bonus += 3;
            //if (e_defend.toward(*it, dest))
            //    bonus += 3;
#if 0
            if (e_explore(dest) < e_explore(*it))
                bonus += exploreBonusMul;
            else if (e_explore(*it) > 9990)  // cul-de sac :(
                bonus += exploreBonusMul * (e_enemies(dest) < e_enemies(*it));
            /// XXX OR WHAT -- fix to use weight, not impassible

            if (e_attack(dest) < e_attack(*it) && e_myHills(*it) > min(16, e_attack(*it))) {
                bonus += attackBonusMul;
                if (e_attack(dest) < 12)
                    bonus += attackBonusMul/2;
            }

            if (e_defend(dest) <= Territory::RANGE * 2 + 1) {
                int defendMul = 15;
                if (e_myHills(dest) < 20)
                    defendMul = 30;
                bonus += defendMul * (e_defend(dest) < e_defend(*it));
            }
#endif

            ant.moves[d].bonus = bonus;
            ant.moves[d].bonus -= 2 * state.grid(dest).byWater;
        }

        PassableAndCanUnsee passable(cantUnsee);
        for (int d = 0; d < TDIRECTIONS; ++d) {
            transform_if(state.visionArc[BEHIND[d]].begin(), state.visionArc[BEHIND[d]].end(),
                      back_inserter(ant.moves[d].lose), Offset(*it), passable);
            for (State::iterator loser = ant.moves[d].lose.begin(); loser != ant.moves[d].lose.end(); ++loser)
                tgrid(*loser)++;
            transform_if(state.visionArc[d].begin(), state.visionArc[d].end(),
                      back_inserter(ant.moves[d].gain), Offset(state.getLocation(*it, d)), passable);
        }
    }
#if 0
#ifdef VISUALIZER
    for (uint i = 0; i < ants.size(); ++i) {
        int d = state.turn % TDIRECTIONS; {
        //for (int d = 0; d < TDIRECTIONS + 1; ++d) {
            Territory::Ant::Move &move = ants[i].moves[d];
            state.v.setFillColor(0, 128, 0, 0.2);
            for (uint g = 0; g < move.gain.size(); ++g)
                state.v.tileBorder(move.gain[g], "MM");
            state.v.setFillColor(128, 0, 0, 0.2);
            for (uint g = 0; g < move.lose.size(); ++g)
                state.v.tileBorder(move.lose[g], "MM");
        }
    }
#endif
#endif

    Territory territory(state, ants, tgrid);
    state.bug << territory;
    Anneal<Territory, 950> anneal;
    Territory best = anneal(territory);
    state.bug << best;

#if 0
#ifdef VISUALIZER
    state.v.setFillColor(128, 0, 0, 0.5);
    for (int row = 0; row < state.rows; ++row)
        for (int col = 0; col < state.cols; ++col)
            if (!cantUnsee[row][col] && state.grid[row][col].isVisible && best.territory[row][col] == 0)
                state.v.tile(Location(row,col));
    state.v.setFillColor(0, 128, 0, 0.5);
    for (int row = 0; row < state.rows; ++row)
        for (int col = 0; col < state.cols; ++col)
            if (!cantUnsee[row][col] && !state.grid[row][col].isVisible && best.territory[row][col] > 0)
                state.v.tile(Location(row,col));
#endif
#endif

    for (uint i = 0; i < best.ants.size(); ++i) {
        static const string why("territory");
        sessile.insert(best.ants[i].loc);
        moves.push(Move(best.ants[i].loc, best.ants[i].dir, 1, -98, why));
    }

}
