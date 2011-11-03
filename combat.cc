#include <iostream>
#include <stdlib.h>

#include "State.h"
#include "Bot.h"
#include "anneal.h"

using namespace std;

class Combat
{
public:
    struct Ant {
        Ant(const Location &l) : loc(l), dir(TDIRECTIONS) { }
        struct Move {
            Move() : occupied(0) { }
            bool *occupied;
            vector<bool> overlap;
        };
        Location loc;
        Move moves[TDIRECTIONS+1];
        int dir;
    };

    struct Enemy {
        Enemy(const Location &l) : loc(l), weakness(0) { }
        Location loc;
        int weakness;
    };

    Combat(State &s, vector<Ant> &a, vector<Enemy> &e)
        : state(s)
        , ants(a)
        , enemies(e)
        , my_e(0)
    {
        for (uint i = 0; i < ants.size(); ++i) {
            apply(i);
        }
        for (uint i = 0; i < enemies.size(); ++i) {
            my_e += score(i);
        }
    }

    Combat(const Combat &other)
        : state(other.state)
        , ants(other.ants)
        , enemies(other.enemies)
        , my_e(other.my_e)
        , undo_i(other.undo_i)
        , undo_dir(other.undo_dir)
    {
    }

    ~Combat()
    {
    }

    double e() const
    {
        return -my_e;
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

    Combat & operator = (const Combat & rhs)
    {
        ants = rhs.ants;
        enemies = rhs.enemies;
        my_e = rhs.my_e;
        undo_i = rhs.undo_i;
        undo_dir = rhs.undo_dir;
        return *this;
    }

private:

    Combat() : state(*((State *)0)) { }

    int score(int n)
    {
        Combat::Enemy &enemy = enemies[n];
        if (enemy.weakness == 0)
            return 1;
        else if (enemy.weakness >= 2)
            return 2;
        else
            return 0;
    }

    void apply(int n)
    {
        Combat::Ant &ant = ants[n];
        Combat::Ant::Move &move = ant.moves[ant.dir];
        *move.occupied = true;
        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i])
                enemies[i].weakness += 1;
        }
    }

    bool change(int n, int dir)
    {
        Combat::Ant &ant = ants[n];
        if (dir == ant.dir)
            return false;
        Combat::Ant::Move &old = ant.moves[ant.dir];
        Combat::Ant::Move &move = ant.moves[dir];
        if (*move.occupied)
            return false;
        *old.occupied = false;
        *move.occupied = true;
        ant.dir = dir;
        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i] != old.overlap[i]) {
                int oldscore = score(i);
                if (old.overlap[i])
                    enemies[i].weakness -= 1;
                else
                    enemies[i].weakness += 1;
                my_e += score(i) - oldscore;
            }
        }
        return true;
    }

    friend ostream & operator << (ostream &os, const Combat &combat);

    State &state;
public:
    vector<Ant> ants;
    vector<Enemy> enemies;
private:
    int my_e;
    int undo_i;
    char undo_dir;
};

ostream & operator << (ostream &os, const Combat &combat)
{
    os << "combat:" << endl << " score: " << combat.e() << endl << " enemies:" << endl;
    for (vector<Combat::Enemy>::const_iterator it = combat.enemies.begin(); it != combat.enemies.end(); ++it) {
        os << "  " << (*it).loc << " w(" << (*it).weakness << ")" << endl;
    }

    os << endl << " ants:" << endl;
    for (vector<Combat::Ant>::const_iterator it = combat.ants.begin(); it != combat.ants.end(); ++it) {
        os << "  " << (*it).loc << " " << CDIRECTIONS[(*it).dir] << endl;
    }
    os << endl;

    return os;
}

/**
 * Identify which ants are in position to be in combat and handle
 * them specially.
 */
void Bot::combat(Move::close_queue &moves, set<Location> &sessile)
{
    double close = state.attackradius + 2;  // 2 moves
    int limit = ceil(state.attackradius) + 2;     // looser manhattan limit

    // Reset a grid of locations for the search to use to determine what
    // locations are occupied.  Initialize it with true for impassible (or
    // undesirable) locations:
    combatOccupied.reset();
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            if (state.grid[r][c].isWater)
                combatOccupied(r, c) = true;
        }
    }

    // Construct *potential* enemies list:  All places where any enemies
    // relevant to our ants could move in the next turn.
    vector<Combat::Enemy> enemies;
    for (State::iterator it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it) {
        if (e_self.euclidean(*it, limit) <= close) {
            for (int d = 0; d < TDIRECTIONS + 1; ++d) {
                const Location dest = state.getLocation(*it, d);
                if (!combatOccupied(dest))
                    enemies.push_back(Combat::Enemy(dest));
            }
        }
    }

    // For every one of our ants that might engage the enemy, compute
    // which of the above potential locations are reachable on the next
    // turn for each possible move.
    vector<Combat::Ant> ants;
    for (State::iterator it = state.myAnts.begin(); it != state.myAnts.end(); ++it) {
        if (e_enemies.euclidean(*it, limit) <= close) {
            ants.push_back(Combat::Ant(*it));
            Combat::Ant &ant = ants.back();
            for (int d = 0; d < TDIRECTIONS + 1; ++d) {
                const Location dest = state.getLocation(*it, d);
                ant.moves[d].occupied = &combatOccupied(dest);
                if (*ant.moves[d].occupied)
                    continue;
                for (uint j = 0; j < enemies.size(); ++j) {
                    bool overlaps = state.distance(enemies[j].loc, dest) <=
                                                   state.attackradius;
                    ant.moves[d].overlap.push_back(overlaps);
                }
            }
        }
    }


    Combat combat(state, ants, enemies);
    state.bug << combat;
    Anneal<Combat> anneal;
    Combat best = anneal(combat);
    state.bug << best;

    for (uint i = 0; i < best.ants.size(); ++i) {
        sessile.insert(best.ants[i].loc);
        moves.push(Move(best.ants[i].loc, best.ants[i].dir, 1, 1));
    }
}
