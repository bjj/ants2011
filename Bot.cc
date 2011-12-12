#include <stdlib.h>
#include <math.h>

#include "Bot.h"

#include "edt.h"
#include "gridbfs.h"

State state;

using namespace std;

//constructor
Bot::Bot()
    : maxVisibleSquares(0)
    , maxVisibleTurn(-1)
    , e_food("food")
    , e_explore("explore", e_myHills)
    , e_push("push", e_myHills)
    , e_attack("attack")
    , e_defend("defend")
    , e_enemies("enemies")
    , e_self("self")
    , e_myHills("home")
    , myInitialAnts(0), myFoodEaten(0), myDeadAnts(0), myNewAntTurn(-1)
{
}

//plays a single game of Ants.
void Bot::playGame()
{
    homeDefense = state.neighborhood_offsets(state.viewradius + 2);

    set<Location> vN_m1(state.visionNeighborhood.begin(), state.visionNeighborhood.end());
    for (int d = 0; d < TDIRECTIONS; ++d)
        for (State::iterator it = state.visionArc[d].begin(); it != state.visionArc[d].end(); ++it)
            vN_m1.erase(*it);
    copy(vN_m1.begin(), vN_m1.end(), back_inserter(visionNeighborhood_m1));

    endTurn();

    //continues making moves while the game is not over
    while(cin >> state)
    {
        if (state.turn == 1) {
            if (state.noPlayers == 0)
                state.noPlayers = 5; // XXX
            state.noHills = state.myHills.size();
            state.avgHillSpacing = sqrt(state.rows * state.cols / state.noPlayers / state.noHills);
            state.bug << state.noHills << " hills, " << state.noPlayers << " players: spacing " << state.avgHillSpacing << endl;
            resetHive();
            maybeInitial();
        }
        state.updateVisionInformation();
        makeMoves();
        endTurn();
    }
}

/**
 * Produce a vector of nodes that are on the frontier of the unexplored
 * area of the map.  This is suitable for EDT to produce a map that
 * encourages exploration.  The distance-to-unexplored values of the
 * Great Unknown will be wrong (they should flood with "1") but we don't
 * care because we're not there!
 */
template <typename Predicate>
vector<Location> Bot::frontier(const Predicate &pred)
{
    vector<Location> frontier;

    for (int r = 0; r < state.rows; ++r) {
        int below = (r + 1) % state.rows;
        int above = (r - 1 + state.rows) % state.rows;
        bool prevSeen = pred(Location(r, state.cols-1));
        bool nextSeen = pred(Location(r,0));
        for (int c = 1; c < state.cols; ++c) {
            bool seen = nextSeen;
            nextSeen = pred(Location(r, c));
            bool aboveSeen = pred(Location(above, c-1));
            bool belowSeen = pred(Location(below, c-1));
            if (!state.grid[r][c-1].isWater && !seen && (prevSeen | nextSeen | aboveSeen | belowSeen)) {
                frontier.push_back(Location(r,c-1));
            }
            prevSeen = seen;
        }
    }
    return frontier;
}

struct Visited {
    bool operator () (const Location &loc) const
    {
        return state.grid(loc).wasVisible;
    }
};

struct Visible {
    bool operator () (const Location &loc) const
    {
        return state.grid(loc).wasVisible;
    }
};

struct SeenRecently {
    SeenRecently(int when) : seenTurn(when) { }
    bool operator () (const Location &loc) const
    {
        return state.grid(loc).lastSeenTurn > seenTurn;
    }
protected:
    int seenTurn;
};

static inline int costInflect(int c, int breakpoint, int mul, int div)
{
    return c <= breakpoint ? c : (c * mul / div);
}

static inline Move
makeMove(const Location &loc, const Edt &edt, int add = 0, int breakpoint = 99999, int mul = 1, int div = 1)
{
    int close = 9999;
    int dir = edt.gradient(loc, &close);
    int score = close <= breakpoint ? close : (close * mul / div);
    score += add;
    return Move(loc, dir, score, close, edt.name);
}

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;

    if (state.visibleSquares > maxVisibleSquares) {
        maxVisibleSquares = state.visibleSquares;
        maxVisibleTurn = state.turn;
    }
    state.bug << "visible " << state.visibleSquares << " out of " << state.rows * state.cols << " or " << double(state.visibleSquares)/state.rows/state.cols << " max " << maxVisibleSquares << endl;

    updateHive();
    maybe();

    e_myHills.update(state.allMyHills.begin(), state.allMyHills.end());

    vector<Location> frontier = this->frontier(SeenRecently(state.turn - 100));
    e_explore.update(frontier.begin(), frontier.end());

    state.bug << state << endl;

    e_enemies.update(state.enemyAnts.begin(), state.enemyAnts.end());
    e_self.update(state.myAnts.begin(), state.myAnts.end());
    e_food.update(state.allFood.begin(), state.allFood.end());

    vector<Location> victims;
    if (state.myAnts.size() > 5) {
        int meekness = 20 - state.myAnts.size();
        for (LocationSet::iterator it = state.allEnemyHills.begin(); it != state.allEnemyHills.end(); ++it) {
            if (e_enemies.empty() || e_enemies(*it) > meekness || e_self(*it) < e_enemies(*it))
                victims.push_back(*it);
        }
    }
#if 0
    for (State::iterator it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it) {
        if (e_myHills(*it) < state.viewradius + 2)
            victims.push_back(*it);
    }
#endif
    e_attack.update(victims.begin(), victims.end());

    busy.reset();
    for(int ant=0; ant<(int)state.myAnts.size(); ant++)
        busy(state.myAnts[ant]) = 1;
    for (uint i = 0; i < state.food.size(); ++i)
        busy(state.food[i]) = 1;

    Move::close_queue moves;
    LocationSet sessile;

    hotspots.clear();
    combat(moves, sessile);
    eat(moves, sessile);

    pushy();

    vector<Location> defense;
    int defenders = 0;
    if (state.turn < state.avgHillSpacing) {
        defenders = 0;
    } else if (state.myAnts.size() < 10) {
        defenders = 0;
    } else {
        defenders = state.myAnts.size() / 10;
    }
    for (State::iterator it = state.myHills.begin(); it != state.myHills.end(); ++it) {
        static const int formation[4][2] = { {-1,-1}, {-1,1}, {1,-1}, {1,1} };

        for (int i = 0; i < 4; ++i) {
            const Location loc = state.deltaLocation(*it, formation[i][0], formation[i][1]);
            const Square &square = state.grid[loc.row][loc.col];
            if (square.isWater)
                continue;

            if (defenders-- > 0) {
                /*
                if (square.ant == 0)
                    sessile.insert(loc);
                if (square.ant != 0)
                    defense.push_back(loc);
                */
                if (e_self(loc) > 4)
                    defense.push_back(loc);
            }
        }
    }

#if 0 // pushy?
    for (State::iterator it = state.myHills.begin(); it != state.myHills.end(); ++it) {
        if (!state.grid(*it).isVisible) {
            defense.push_back(*it);
            continue;
        }
        if (e_enemies(*it) > state.viewradius * 2)
            continue;
        for (State::iterator dt = homeDefense.begin(); dt != homeDefense.end(); ++dt) {
            Location loc = state.deltaLocation(*it, (*dt).row, (*dt).col);
            if (e_enemies(loc) != 1)
                continue;
            int steps = 1, dir, close = 1;
            do {
                dir = e_myHills.gradient(loc, &close);
                if (dir != -1)
                    loc = state.getLocation(loc, dir);
                if (e_self(loc) <= steps)
                    break;
                steps++;
            } while (steps < 20 && dir != -1 && close != 1);
            defense.push_back(loc);
        }
    }
#endif

    copy(hotspots.begin(), hotspots.end(), back_inserter(defense));
    for (State::iterator it = defense.begin(); it != defense.end(); ++it)
        state.v.star(*it, 0.4, 0.8, 6, false);
    e_defend.update(defense.begin(), defense.end());

    territory(moves, sessile);

    visualize();

    Move::close_queue retry;
    bool moved = false;
    int angle = 0;
    //int avoid = moves.size() < 5 ? (state.attackradius + 2) : 0;
    static const int *rotate[] = { AHEAD, RIGHT, LEFT };
    while (!moves.empty()) {
        const Move &move = moves.top();
        int dir = rotate[angle][move.dir];
        Location new_loc = state.getLocation(move.loc, dir);
        const Square &square = state.grid(new_loc);
        if (move.close < 0 || !busy(new_loc) && !square.isWater && !square.hillPlayer == 0) {
            state.bug << "move " << move.loc << ": " << CDIRECTIONS[dir] << " [" << *move.why << "] a" << angle << endl;
            if (dir != TDIRECTIONS)
                state.makeMove(move.loc, dir);
            busy(move.loc)--;
            busy(new_loc)++;
            moved = true;
        } else {
            retry.push(move);
        }
        moves.pop();
        if (moves.empty()) {
            if (moved || ++angle < 3)
                swap(retry, moves);
            moved = false;
        }
    }

    state.bug << "time taken: " << state.timer.getTime() << "ms" << endl << endl;
}

void Bot::eat(Move::close_queue &moves, LocationSet &sessile)
{
    GridBfs<Passable> end;

    state.v.setLineColor(0,0,200);

    // even at the beginning moving out of your bunker is good
    float explore = 0.25; // state.myAnts.size() > 4 ? 0.25 : 0.0;
    priority_queue<pair<float, Location> > food;
    for (LocationSet::iterator it = state.allFood.begin(); it != state.allFood.end(); ++it) {
        float score = 0;
        score += -e_self(*it);
        score += 1 * state.grid(*it).isVisible;
        score += 2 * (e_enemies(*it) > state.viewradius);
        score += -5 * (e_enemies(*it) <= e_self(*it));
        score += explore * max(0.0, state.viewradius - e_explore(*it));
        food.push(make_pair(score, *it));
    }
    LocationSet ontheway;
    while (!food.empty()) {
        const Location loc = food.top().second;
        food.pop();
        GridBfs<Passable> bfs(loc);
        for(++bfs; bfs != end; ++bfs) {
            const Square &square = state.grid(*bfs);
            //if (square.ant == 0 && e_food(*bfs) == bfs.distance()+1 && !sessile.count(*bfs)) {
            if (ontheway.count(*bfs)) {
                break;
            } else if (square.ant == 0 && !sessile.count(*bfs) && !e_attack(*bfs) < 8) {
                static const string why("food+");
                if (bfs.distance() != 1)
                    moves.push(Move(*bfs, bfs.direction(), 1, 1, why));
                sessile.insert(*bfs);
                ontheway.insert(state.getLocation(*bfs, bfs.direction()));
                state.v.arrow(loc, *bfs);
                break;
            } else if (square.isFood) {
                // other food is closer to all ants
                //break;
            } else if (square.ant > 0) {
                // someone else closer
                //break;
            } else if (bfs.distance() > 35) {
                // too far
                break;
            }
        }
    }
}

//finishes the turn
void Bot::endTurn()
{
    if(state.turn > 0)
        state.reset();
    state.turn++;

    cout << "go" << endl;
}

int Bot::hive() const
{
    return myInitialAnts + myFoodEaten - state.myAnts.size() - myDeadAnts;
}

void Bot::resetHive()
{
    myInitialAnts = state.myAnts.size();
    myFoodEaten = 0;
    myDeadAnts = 0;
}

void Bot::updateHive()
{
    myDeadAnts += state.diedByPlayer[0];

    for (State::iterator it = state.myHills.begin(); it != state.myHills.end(); ++it) {
        if (state.grid(*it).ant == 0) {
            myNewAntTurn = state.turn;  // approx
            break;
        }
    }

    if (state.turn - myNewAntTurn > 3) {// really just 1 or 2
        if (hive() != 0)
            state.bug << "hive thinks I'm dead?" << endl;
        resetHive();
    }

    myFoodEaten += state.ateByPlayer[0];
    state.bug << "hive " << hive() << " = " << myInitialAnts << " + " << myFoodEaten << " - " << myDeadAnts << " - " << state.myAnts.size() << endl;
}
