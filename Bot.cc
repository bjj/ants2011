#include <stdlib.h>
#include <math.h>

#include "Bot.h"

#include "edt.h"
#include "gridbfs.h"

using namespace std;

//constructor
Bot::Bot()
    : e_food("food", state)
    , e_explore("explore", state)
    , e_revisit("revisit", state)
    , e_attack("attack", state)
    , e_defend("defend", state)
    , e_enemies("enemies", state)
    , e_self("self", state, false)
    , e_myHills("home", state, false)
    , busy()
    , combatOccupied()
{
}

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;

    if (state.rows == 0 || state.cols == 0)
        exit(0);

    srandom(state.seed);
    srand48(state.seed);

    state.setup();
    endTurn();

    busy.init(state);
    combatOccupied.init(state);

    for (int i = 0; i < 10; ++i) {
        interesting.push_back(Location(random() % state.rows, random() % state.cols));
    }

    //continues making moves while the game is not over
    while(cin >> state)
    {
        if (state.turn == 1) {
            if (state.noPlayers == 0)
                state.noPlayers = 5; // not sent by server yet
            state.noHills = state.myHills.size();
            state.avgHillSpacing = sqrt(state.rows * state.cols / state.noPlayers / state.noHills);
            state.bug << state.noHills << " hills, " << state.noPlayers << " players: spacing " << state.avgHillSpacing << endl;
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
vector<Location> Bot::frontier()
{
    Predicate pred;
    vector<Location> frontier;

    for (int r = 0; r < state.rows; ++r) {
        int below = (r + 1) % state.rows;
        int above = (r - 1 + state.rows) % state.rows;
        bool prevSeen = pred(state.grid[r][state.cols-1]);
        bool nextSeen = pred(state.grid[r][0]);
        for (int c = 1; c < state.cols; ++c) {
            bool seen = nextSeen;
            nextSeen = pred(state.grid[r][c]);
            bool aboveSeen = pred(state.grid[above][c-1]);
            bool belowSeen = pred(state.grid[below][c-1]);
            if (!state.grid[r][c-1].isWater && !seen && (prevSeen | nextSeen | aboveSeen | belowSeen)) {
                frontier.push_back(Location(r,c));
            }
            prevSeen = seen;
        }
    }
    return frontier;
}

struct Visited {
    bool operator () (const Square &square) const
    {
        return square.wasVisible;
    }
};

struct Visible {
    bool operator () (const Square &square) const
    {
        return square.isVisible;
    }
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

Move Bot::pickMove(const Location &loc) const
{
    Move::score_queue pick;
    //pick.push(makeMove(loc, e_food, 0, 8, 5, 2));
    pick.push(makeMove(loc, e_explore));
    pick.push(makeMove(loc, e_revisit, 20, 0, 2, 1));
    pick.push(makeMove(loc, e_attack, 0, 20, 2, 3));
    pick.push(makeMove(loc, e_defend, 0, 8, 2, 1));
    return pick.top();
}

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;

    vector<Location> frontier = this->frontier<Visited>();
    e_explore.update(frontier.begin(), frontier.end());
    frontier = this->frontier<Visible>();
    e_revisit.update(frontier.begin(), frontier.end());

    state.bug << state << endl;
    //state.bug << e_explore << endl;

    e_enemies.update(state.enemyAnts.begin(), state.enemyAnts.end());
    e_self.update(state.myAnts.begin(), state.myAnts.end());
    e_food.update(state.allFood.begin(), state.allFood.end());
    e_myHills.update(state.allMyHills.begin(), state.allMyHills.end());

    vector<Location> victims;
    if (state.myAnts.size() > 5) {
        int meekness = 20 - state.myAnts.size();
        for (set<Location>::iterator it = state.allEnemyHills.begin(); it != state.allEnemyHills.end(); ++it) {
            if (e_enemies.empty() || e_enemies(*it) > meekness)
                victims.push_back(*it);
        }
    }
    for (State::iterator it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it) {
        if (e_myHills(*it) < state.viewradius + 2)
            victims.push_back(*it);
    }
    e_attack.update(victims.begin(), victims.end());

    busy.reset();
    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        busy(state.myAnts[ant]) = true;
    }

    Move::close_queue moves;
    set<Location> sessile;

    combat(moves, sessile);
    eat(moves, sessile);

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
            //bool urgent = e_enemies(loc) <= 8;
            bool urgent = false;

            if (urgent || defenders-- > 0) {
                if (square.ant == 0)
                    sessile.insert(loc);
                if (square.ant != 0 || urgent)
                    defense.push_back(loc);
            }
        }
    }
    e_defend.update(defense.begin(), defense.end());

    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        const Location & loc = state.myAnts[ant];

        if (sessile.count(loc))
            continue;

        const Move m = pickMove(loc);

        state.bug << "ant " << ant << " " << loc << ": " << CDIRECTIONS[m.dir] << " (" << m.close << ")" << endl;

        if (m.dir >= 0)
            moves.push(m);
    }

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
        if (!busy(new_loc) && !square.isWater && !square.hillPlayer == 0) {
            state.bug << "move " << move.loc << ": " << CDIRECTIONS[dir] << " [" << *move.why << "]" << endl;
            state.makeMove(move.loc, dir);
            busy(move.loc) = false;
            busy(new_loc) = true;
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

void Bot::eat(Move::close_queue &moves, set<Location> &sessile)
{
    Passable passable(state);
    GridBfs<Passable> end;

    state.v.setLineColor(0,0,200);

    float explore = state.myAnts.size() > 4 ? 0.25 : 0.0;
    priority_queue<pair<float, Location> > food;
    for (set<Location>::iterator it = state.allFood.begin(); it != state.allFood.end(); ++it) {
        float score = 0;
        score += -e_self(*it);
        score += 1 * state.grid(*it).isVisible;
        score += 2 * (e_enemies(*it) > state.viewradius);
        score += -5 * (e_enemies(*it) <= e_self(*it));
        score += explore * max(0.0, state.viewradius - e_explore(*it));
        food.push(make_pair(score, *it));
    }
    set<Location> ontheway;
    while (!food.empty()) {
        const Location loc = food.top().second;
        food.pop();
        GridBfs<Passable> bfs(state.grid, passable, loc);
        for(++bfs; bfs != end; ++bfs) {
            const Square &square = state.grid(*bfs);
            //if (square.ant == 0 && e_food(*bfs) == bfs.distance()+1 && !sessile.count(*bfs)) {
            if (ontheway.count(*bfs)) {
                break;
            } else if (square.ant == 0 && !sessile.count(*bfs)) {
                static const string why("food+");
                moves.push(Move(*bfs, bfs.direction(), 1, 1, why));
                sessile.insert(*bfs);
                ontheway.insert(state.getLocation(*bfs, bfs.direction()));
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
        state.v.arrow(loc, *bfs);
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
