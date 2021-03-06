#include <iostream>
#include <stdlib.h>
#include <stdint.h>

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
            if (weakness > antWeakness)
                return bonus;
            else if (weakness == antWeakness)
                return bonus / 2;
            else
                return 0;
        }
    };

    struct Ant {
        Ant(const Location &l) : loc(l), dir(TDIRECTIONS), weakness(0), enemyWeakness(99), cost(250), noOverlaps(false) { }
        struct Move {
            Move() : occupied(0), bonus(0) { }
            bool *occupied;
            int bonus;
            vector<char> overlap;
        };
        Location loc;
        Move moves[TDIRECTIONS+1];
        int dir;
        int weakness, enemyWeakness;
        int cost;
        bool noOverlaps;

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
        for (uint i = 0; i < enemies.size(); ++i) {
            if (move.overlap[i])
                enemies[i].antWeakness = min(enemies[i].antWeakness, ant.weakness);
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
        defend_score -= ant.score();
        ant.dir = dir;

        if (!ant.noOverlaps) {
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
        os << "  " << (*it).loc << " w(" << (*it).weakness << ":" << (*it).antWeakness << ") b(" << (*it).bonus << ") s(" << (*it).score() << ")" << endl;
    }

    os << endl << " ants:" << endl;
    for (vector<Combat::Ant>::const_iterator it = combat.ants.begin(); it != combat.ants.end(); ++it) {
        os << "  " << (*it).loc << " " << CDIRECTIONS[(*it).dir] << " w(" << (*it).weakness << ":" << (*it).enemyWeakness <<") b(" << (*it).moves[(*it).dir].bonus << ")" << " s(" << (*it).score() << ")" << endl;
    }
    os << endl;

    return os;
}

// tiny union-find implementation
static int ufFind(vector<int> &equiv, int a)
{
    if (equiv[a] == a)
        return a;
    else
        return (equiv[a] = ufFind(equiv, equiv[a]));
}

// tiny union-find implementation
static void ufUnion(vector<int> &equiv, int a, int b)
{
    a = ufFind(equiv, a);
    b = ufFind(equiv, b);
    if (a == b)
        return;
    if (a < b)
        swap(a, b);
    equiv[a] = b;
}

void Bot::combatLabel(vector<int> &equiv, int &nextLabel, const vector<Location> &ants)
{
    for (vector<Location>::const_iterator it = ants.begin(); it != ants.end(); ++it, ++nextLabel) {
        equiv[nextLabel] = nextLabel; // initially an island
        for (State::iterator ct = state.combatNeighborhood.begin(); ct != state.combatNeighborhood.end(); ++ct) {
            int &label = combatLabels(state.deltaLocation(*it, (*ct).row, (*ct).col));
            if (label != 0)
                ufUnion(equiv, label, nextLabel); // join the party
            else
                label = nextLabel;
        }
    }
}

/**
 * Given two lists of ants find which ones threaten the other
 */
vector<Location>
Bot::combatThreat(const vector<Location> &ants, const vector<Location> &enemies,  Grid<bool> &threat)
{
    const vector<Location> &neighborhood = state.combatNeighborhood;
    threat.reset();
    paint(threat, enemies.begin(), enemies.end(),
        neighborhood.begin(), neighborhood.end());

    vector<Location> result;
    result.reserve(ants.size());
    for (vector<Location>::const_iterator it = ants.begin(); it != ants.end(); ++it) {
        if (threat(*it))
            result.push_back(*it);
    }
    return result;
}

/**
 * Group ants into islands of isolated combat
 */
void Bot::combat(Move::close_queue &moves, LocationSet &sessile)
{
    // Reset a grid of locations for the search to use to determine what
    // locations are occupied.  Initialize it with true for impassible (or
    // undesirable) locations:
    combatOccupied.reset();
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            const Square &square = state.grid[r][c];
            if (square.isWater || square.isFood || square.ant == 0)
                combatOccupied(r, c) = true;
        }
    }

    vector<Location> ants = combatThreat(state.myAnts, state.enemyAnts, enemyThreat);
    vector<Location> enemies = combatThreat(state.enemyAnts, state.myAnts, selfThreat);

    enemyNextThreat.reset();
    paint(enemyNextThreat, enemies.begin(), enemies.end(),
        combatNeighborhood_p1.begin(), combatNeighborhood_p1.end());

    // Make sure ants next to combat ants are in the queue so they can jiggle out of the way
    LocationSet neighbors;
    for (State::iterator it = ants.begin(); it != ants.end(); ++it) {
        neighbors.insert(*it);
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                const Location dest = state.deltaLocation(*it, dr, dc);
                if (state.grid(dest).ant == 0)
                    neighbors.insert(dest);
            }
        }
    }
    ants.clear();
    copy(neighbors.begin(), neighbors.end(), back_inserter(ants));

    combatLabels.reset();
    vector<int> equiv(state.myAnts.size() + state.enemyAnts.size() + 1);
    int nextLabel = 1;
    combatLabel(equiv, nextLabel, ants);
    combatLabel(equiv, nextLabel, enemies);

    vector<pair<vector<Location>, vector<Location> > > groups(nextLabel);
    for (State::iterator it = ants.begin(); it != ants.end(); ++it)
        groups[ufFind(equiv, combatLabels(*it))].first.push_back(*it);
    for (State::iterator it = enemies.begin(); it != enemies.end(); ++it)
        groups[ufFind(equiv, combatLabels(*it))].second.push_back(*it);

    static uint8_t colors[8][3] = {
        { 0, 0, 0 },
        { 128, 0, 0 },
        { 0, 128, 0 },
        { 128, 128, 0 },
        { 0, 0, 128 },
        { 128, 0, 128 },
        { 0, 128, 128 },
        { 128, 128, 128 },
    };
    for (int c = 0, i = 1; i < nextLabel; ++i) {
        state.bug << "group " << i << " " << groups[i].first.size() << " / " << groups[i].second.size() << endl;
        if (groups[i].first.empty())
            continue;
        state.v.setLineColor(colors[c][0], colors[c][1], colors[c][2],0.5);
        c = (c + 1) % 8;
        combatGroup(moves, sessile, groups[i].first, groups[i].second);
    }
}

/**
 * Identify which ants are in position to be in combat and handle
 * them specially.
 */
void Bot::combatGroup(Move::close_queue &moves, LocationSet &sessile, const vector<Location> &ants_l, const vector<Location> &enemies_l)
{
#if 0
#ifdef VISUALIZER
    for (vector<Location>::const_iterator it = enemies_l.begin(); it != enemies_l.end(); ++it)
        state.v.tileBorder(*it, "MM");
    for (vector<Location>::const_iterator it = ants_l.begin(); it != ants_l.end(); ++it)
        state.v.tileBorder(*it, "MM");
#endif
#endif

    int stillThresh = 2 + random() % 3;

    if (enemies_l.size() + 2 >= ants_l.size()) { // ~outnumbered
        if (enemies_l.size() == 1) {
            hotspots.push_back(enemies_l[0]);
            hotspots.push_back(enemies_l[0]);
        } else {
            vector<pair<int, Location> > sorted;
            transform(enemies_l.begin(), enemies_l.end(), back_inserter(sorted), DistanceTag(e_myHills));
            sort(sorted.begin(), sorted.end());
            for (int n = min(sorted.size(), ants_l.size() - enemies_l.size() + 2), i = 0; i < n; ++i)
                hotspots.push_back(sorted[i].second);
        }
    }

    // Construct *potential* enemies list:  All places where any enemies
    // relevant to our ants could move in the next turn.
    vector<Combat::Enemy> enemies;
    for (vector<Location>::const_iterator it = enemies_l.begin(); it != enemies_l.end(); ++it) {
        int id = it - enemies_l.begin();
        bool still = state.grid(*it).stationary > stillThresh;
        for (int d = still ? TDIRECTIONS : 0; d < TDIRECTIONS + 1; ++d) {
            const Location dest = state.getLocation(*it, d);
            const Square &square = state.grid(dest);
            if (!square.isWater && !square.isFood) {
                enemies.push_back(Combat::Enemy(dest, id));
                // note that adding bonus here trades guaranteed death against possible kill
                enemies.back().bonus += min(250, state.grid(*it).stationary * 3);
                static const int nearBonus[] = {2000, 1200, 720, 431, 259, 155, 93, 55, 33, 20, 12, 7};
                if (e_myHills(dest) < int(sizeof(nearBonus)/sizeof(nearBonus[0])))
                    enemies.back().bonus += nearBonus[e_myHills(dest)];
                else if (e_self(dest) <= 3)
                    enemies.back().bonus /= 4; // not likely coming this way!
            }
        }
    }

    int advantage = max(0, int(ants_l.size() - enemies_l.size()) - 2);

    // For every one of our ants that might engage the enemy, compute
    // which of the above potential locations are reachable on the next
    // turn for each possible move.
    vector<Combat::Ant> ants;
    for (vector<Location>::const_iterator it = ants_l.begin(); it != ants_l.end(); ++it)
        combatOccupied(*it) = false;
    for (vector<Location>::const_iterator it = ants_l.begin(); it != ants_l.end(); ++it) {
        ants.push_back(Combat::Ant(*it));
        Combat::Ant &ant = ants.back();
        bool anyOverlaps = false;
        for (int d = 0; d < TDIRECTIONS + 1; ++d) {
            const Location dest = state.getLocation(*it, d);
            ant.moves[d].occupied = &combatOccupied(dest);
            if (*ant.moves[d].occupied)
                continue;
            for (uint j = 0; j < enemies.size(); ++j) {
                bool overlaps = state.distance(enemies[j].loc, dest) <=
                                               state.attackradius;
                ant.moves[d].overlap.push_back(overlaps);
                anyOverlaps |= overlaps;
            }

            if (state.grid[dest.row][dest.col].hillPlayer > 0)
                ant.moves[d].bonus += state.grid[dest.row][dest.col].ant == -1 ? 100 : -250;

            if (e_attack(dest) < e_myHills(dest)) {
                if (e_explore.toward(*it, dest))
                    ant.moves[d].bonus += 1;

                if (e_frontline.toward(*it, dest))
                    ant.moves[d].bonus += 1;

                if (e_attack.toward(*it, dest))
                    ant.moves[d].bonus += 2;
            } else {
                if (enemyThreat(dest) && e_myHills.toward(*it, dest))
                    ant.moves[d].bonus += 6;
            }

            if (advantage && e_enemies.toward(*it, dest))
                ant.moves[d].bonus += advantage * 10;

            if (enemyThreat(dest))
                ant.moves[d].bonus += 7;
            else if (enemyNextThreat(dest))
                ant.moves[d].bonus += 4;

            ant.moves[d].bonus -= state.grid(dest).byWater;

            // mainly for single ants
            if (e_food(dest) < 20 && e_food.toward(*it, dest))
                ant.moves[d].bonus += 5;
        }
        if (!anyOverlaps) {
            ant.noOverlaps = true;
        }

        if (state.myAnts.size() > 250)
            ant.cost -= 150;
        else if (state.myAnts.size() > 200)
            ant.cost -= 75;
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
