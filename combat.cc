#include <iostream>
#include <stdlib.h>

#include "State.h"
#include "Bot.h"
#include "anneal.h"

using namespace std;

class Combat
{
public:
    struct Enemy {
        Enemy(const Location &l, int n) : loc(l), id(n), bonus(50), weakness(0), antWeakness(99) { }
        Location loc;
        int id;
        int bonus;
        int weakness, antWeakness;

        inline int score() const
        {
            return bonus * (weakness > antWeakness);
        }
    };

    struct Ant {
        Ant(const Location &l) : loc(l), dir(TDIRECTIONS), weakness(0), enemyWeakness(99), cost(250) { }
        struct Move {
            Move() : occupied(0), bonus(0) { }
            bool *occupied;
            int bonus;
            vector<bool> overlap;
        };
        Location loc;
        Move moves[TDIRECTIONS+1];
        int dir;
        int weakness, enemyWeakness;
        int cost;

        inline int score() const
        {
            return moves[dir].bonus + -cost * (weakness >= enemyWeakness);
        }

        struct Weakness {
            Weakness(Ant &a) :ant(a), lastEnemyId(-1), weakness(0), enemyWeakness(99) { }
            Ant &ant;
            int lastEnemyId;
            int weakness, enemyWeakness;

            inline void attack(const Enemy &enemy)
            {
                if (lastEnemyId != enemy.id) {
                    lastEnemyId = enemy.id;
                    weakness++;
                }
                enemyWeakness = min(enemyWeakness, enemy.weakness);
            }
            inline void apply()
            {
                ant.weakness = weakness;
                ant.enemyWeakness = enemyWeakness;
            }
        };
    };

    Combat(State &s, vector<Ant> &a, vector<Enemy> &e)
        : state(s)
        , ants(a)
        , enemies(e)
        , attack_score(0)
        , defend_score(0)
    {
        for (uint i = 0; i < ants.size(); ++i)
            apply(i);
        for (uint i = 0; i < ants.size(); ++i) {
            apply2(i);
            defend_score += ants[i].score();
        }
        for (uint i = 0; i < enemies.size(); ++i) {
            attack_score += enemies[i].score();
        }
    }

    Combat(const Combat &other)
        : state(other.state)
        , ants(other.ants)
        , enemies(other.enemies)
        , attack_score(other.attack_score)
        , defend_score(other.defend_score)
        , undo_i(other.undo_i)
        , undo_dir(other.undo_dir)
    {
    }

    ~Combat()
    {
    }

    double e() const
    {
        return -(attack_score + defend_score);
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
        attack_score = rhs.attack_score;
        defend_score = rhs.defend_score;
        undo_i = rhs.undo_i;
        undo_dir = rhs.undo_dir;
        return *this;
    }

private:

    Combat() : state(*((State *)0)) { }

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

    void apply2(int n)
    {
        Combat::Ant &ant = ants[n];
        Combat::Ant::Move &move = ant.moves[ant.dir];
        Combat::Ant::Weakness vs(ant);
        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i])
                vs.attack(enemies[i]);
        }
        vs.apply();
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
        defend_score -= ant.score();
        ant.dir = dir;

        Combat::Ant::Weakness vs(ant);
        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i] != old.overlap[i]) {
                attack_score -= enemies[i].score();
                if (old.overlap[i])
                    enemies[i].weakness -= 1;
                else
                    enemies[i].weakness += 1;
            }
            if (move.overlap[i])
                vs.attack(enemies[i]);
        }
        vs.apply();

        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i] != old.overlap[i]) {
                int antWeakness = 99;
                for (uint j = 0; j < ants.size(); ++j) {
                    Combat::Ant &ant2 = ants[j];
                    Combat::Ant::Move &move2 = ant2.moves[ant2.dir];
                    if (!move2.overlap[i])
                        continue;
                    antWeakness = min(ant2.weakness, antWeakness);

                    defend_score -= ant2.score();
                    Combat::Ant::Weakness vs2(ant2);
                    for (uint k = 0; k < enemies.size(); ++k) {
                        if (!move2.overlap[k])
                            continue;
                        vs2.attack(enemies[k]);
                    }
                    vs2.apply();
                    defend_score += ant2.score();
                }
                enemies[i].antWeakness = antWeakness;
                attack_score += enemies[i].score();
            }
        }
        defend_score += ant.score();
        return true;
    }

    friend ostream & operator << (ostream &os, const Combat &combat);

    State &state;
public:
    vector<Ant> ants;
    vector<Enemy> enemies;
private:
    int attack_score;
    int defend_score;
    int undo_i;
    char undo_dir;
};

ostream & operator << (ostream &os, const Combat &combat)
{
    os << "combat:" << endl << " score: " << -combat.e() << endl << " enemies:" << endl;
    for (vector<Combat::Enemy>::const_iterator it = combat.enemies.begin(); it != combat.enemies.end(); ++it) {
        os << "  " << (*it).loc << " w(" << (*it).weakness << ")" << endl;
    }

    os << endl << " ants:" << endl;
    for (vector<Combat::Ant>::const_iterator it = combat.ants.begin(); it != combat.ants.end(); ++it) {
        os << "  " << (*it).loc << " " << CDIRECTIONS[(*it).dir] << " w(" << (*it).weakness << ":" << (*it).enemyWeakness <<") b(" << (*it).moves[(*it).dir].bonus << ")" << " s(" << (*it).score() << ")" << endl;
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
    double close = state.attackradius + 2.5;  // 2 moves
    int limit = ceil(state.attackradius) + 3;     // looser manhattan limit

    // Reset a grid of locations for the search to use to determine what
    // locations are occupied.  Initialize it with true for impassible (or
    // undesirable) locations:
    combatOccupied.reset();
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            const Square &square = state.grid[r][c];
            if (square.isWater || square.isFood)
                combatOccupied(r, c) = true;
        }
    }

    // Construct *potential* enemies list:  All places where any enemies
    // relevant to our ants could move in the next turn.
    vector<Combat::Enemy> enemies;
    for (State::iterator it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it) {
        //if (e_self.euclidean(*it, limit) <= close) {
        if (ep_self(*it) <= limit) {  // XXX
            int id = it - state.enemyAnts.begin();
            bool still = state.grid(*it).stationary > 4;
            bool timedout = state.grid(*it).stationary > 20;
            if (still)
                state.bug << "stationary: " << *it << endl;
            for (int d = still ? TDIRECTIONS : 0; d < TDIRECTIONS + 1; ++d) {
                const Location dest = state.getLocation(*it, d);
                if (!combatOccupied(dest)) {
                    enemies.push_back(Combat::Enemy(dest, id));
                    if (e_myHills(dest) < 6)
                        enemies.back().bonus *= 20;
                    else if (e_myHills(dest) < 12)
                        enemies.back().bonus *= 2;
                    if (e_attack(dest) < 5)
                        enemies.back().bonus *= 2;
                    if (timedout)
                        enemies.back().bonus *= 4;
                }
            }
        }
    }

    // For every one of our ants that might engage the enemy, compute
    // which of the above potential locations are reachable on the next
    // turn for each possible move.
    vector<Combat::Ant> ants;
    for (State::iterator it = state.myAnts.begin(); it != state.myAnts.end(); ++it) {
        // if (e_enemies.euclidean(*it, limit) <= close) {
        if (ep_enemies(*it) <= limit) {  // XXX
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
                if (state.grid[dest.row][dest.col].hillPlayer > 0)
                    ant.moves[d].bonus += state.grid[dest.row][dest.col].ant == -1 ? 100 : 50;
                if (e_attack(dest) < 3)
                    ant.moves[d].bonus += 50;
                if (e_attack(dest) < e_attack(*it))
                    ant.moves[d].bonus += 20;
                if (e_food(dest) < 20 && e_food(dest) < e_food(*it))
                    ant.moves[d].bonus += 10;
                if (e_attack(dest) < 5 && e_attack(dest) < e_attack(*it))
                    ant.moves[d].bonus += 10;
            }
            ant.moves[TDIRECTIONS].bonus += 5;
            if (state.myAnts.size() > 150)
                ant.cost -= 150;
            else if (e_myHills(ant.loc) > 10 && e_myHills(ant.loc) < 50)
                ant.cost -= 75;
        } else {
            combatOccupied(*it) = true;
        }
    }


    Combat combat(state, ants, enemies);
    state.bug << combat;
    Anneal<Combat> anneal;
    Combat best = anneal(combat);
    state.bug << best;

    for (uint i = 0; i < ants.size(); ++i)
        combatOccupied(ants[i].loc) = false;
    for (uint i = 0; i < best.ants.size(); ++i) {
        static const string why("combat");
        sessile.insert(best.ants[i].loc);
        combatOccupied(state.getLocation(best.ants[i].loc, best.ants[i].dir)) = true;
        moves.push(Move(best.ants[i].loc, best.ants[i].dir, 1, -100, why));
    }

#ifdef TEST_COMBAT_IMPROVE
// XXX this is probably wrong due to combatOccupied
    double score = -best.e();
    best = anneal(combat);
    double score2 = -best.e();
    if (score2 > score)
        state.bug << "IMPROVE over " << score << endl << best;
#endif /* TEST_COMBAT_IMPROVE */

}
