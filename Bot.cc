#include <stdlib.h>
#include <math.h>

#include "Bot.h"

#include "edt.h"

using namespace std;

//constructor
Bot::Bot()
    : e_food("food", state)
    , e_explore("explore", state)
    , e_revisit("revisit", state)
    , e_attack("attack", state)
    , e_defend("defend", state)
    , e_enemies("enemies", state)
    , e_self("self", state)
    , busy(state)
    , combatOccupied(state)
{
    srandom(time(0));
    srand48(time(0));
}

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;

    if (state.rows == 0 || state.cols == 0)
        exit(0);

    state.setup();
    endTurn();

    for (int i = 0; i < 10; ++i) {
        interesting.push_back(Location(random() % state.rows, random() % state.cols));
    }

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.updateVisionInformation();
        makeMoves();
        endTurn();
    }
}

/**
 * Produce a queue of nodes that are on the frontier of the unexplored
 * area of the map.  This is suitable for EDT to produce a map that
 * encourages exploration.  The distance-to-unexplored values of the
 * Great Unknown will be wrong (they should flood with "1") but we don't
 * care because we're not there!
 */
template <typename Predicate>
queue<Location> Bot::frontier()
{
    Predicate pred;
    queue<Location> frontier;

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
                frontier.push(Location(r,c));
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
    pick.push(makeMove(loc, e_food, 0, 8, 5, 2));
    pick.push(makeMove(loc, e_explore));
    pick.push(makeMove(loc, e_revisit, 8, 0, 2, 1));
    pick.push(makeMove(loc, e_attack, 0, 20, 2, 3));
    pick.push(makeMove(loc, e_defend, 0, 4, 2, 1));
    return pick.top();
}

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;

    queue<Location> frontier = this->frontier<Visited>();
    e_explore.update(frontier);
    frontier = this->frontier<Visible>();
    e_revisit.update(frontier);

    state.bug << state << endl;
    //state.bug << e_explore << endl;

    queue<Location> enemies;
    for (State::iterator it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it)
        enemies.push(*it);
    e_enemies.update(enemies);

    queue<Location> myants;
    for (State::iterator it = state.myAnts.begin(); it != state.myAnts.end(); ++it)
        myants.push(*it);
    e_self.update(myants);

    queue<Location> food;
    for (set<Location>::iterator it = state.allFood.begin(); it != state.allFood.end(); ++it)
        food.push(*it);
    e_food.update(food);

    queue<Location> victims;
    if (state.myAnts.size() > 5) {
        int meekness = 20 - state.myAnts.size();
        for (set<Location>::iterator it = state.allEnemyHills.begin(); it != state.allEnemyHills.end(); ++it) {
            if (e_enemies.empty() || e_enemies(*it) > meekness)
                victims.push(*it);
        }
    }
    e_attack.update(victims);

    busy.reset();
    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        busy(state.myAnts[ant]) = true;
    }
    Move::close_queue moves;

    set<Location> sessile;

    combat(moves, sessile);

    queue<Location> defense;
    int defenders = state.myAnts.size() - 4;
    for (State::iterator it = state.myHills.begin(); defenders > 0 && it != state.myHills.end(); ++it) {
        static const int formation[4][2] = { {-1,-1}, {-1,1}, {1,-1}, {1,1} };

        for (int i = 0; defenders > 0 && i < 4; ++i) {
            const Location loc = state.deltaLocation(*it, formation[i][0], formation[i][1]);
            const Square &square = state.grid[loc.row][loc.col];

            if (square.ant == 0)
                sessile.insert(loc);
            else if (!square.isWater)
                defense.push(loc);
            defenders--;
        }
    }
    e_defend.update(defense);

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
        //if (!busy(new_loc) && e_food(new_loc) != 9999 && e_enemies(new_loc) > avoid) {
        if (!busy(new_loc) && e_food(new_loc) != 9999) {
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

//finishes the turn
void Bot::endTurn()
{
    if(state.turn > 0)
        state.reset();
    state.turn++;

    cout << "go" << endl;
}
