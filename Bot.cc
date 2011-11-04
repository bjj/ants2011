#include <stdlib.h>
#include <math.h>

#include "Bot.h"

#include "edt.h"

using namespace std;

//constructor
Bot::Bot()
    : e_food(state)
    , e_explore(state)
    , e_attack(state)
    , e_defend(state)
    , e_enemies(state)
    , e_self(state)
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
queue<Location> Bot::visited_frontier()
{
    queue<Location> frontier;

    for (int r = 0; r < state.rows; ++r) {
        int below = (r + 1) % state.rows;
        int above = (r - 1 + state.rows) % state.rows;
        bool prevSeen = state.grid[r][state.cols-1].wasVisible;
        bool nextSeen = state.grid[r][0].wasVisible;
        for (int c = 1; c < state.cols; ++c) {
            bool seen = nextSeen;
            nextSeen = state.grid[r][c].wasVisible;
            bool aboveSeen = state.grid[above][c-1].wasVisible;
            bool belowSeen = state.grid[below][c-1].wasVisible;
            if (!state.grid[r][c-1].isWater && !seen && (prevSeen | nextSeen | aboveSeen | belowSeen)) {
                frontier.push(Location(r,c));
                state.grid[r][c-1].frontier = 1;
            }
            prevSeen = seen;
        }
    }
    return frontier;
}

static inline int costInflect(int c, int breakpoint, int mul, int div)
{
    return c <= breakpoint ? c : (c * mul / div);
}

static inline Move
makeMove(const Location &loc, const Edt &edt, int breakpoint = 99999, int mul = 1, int div = 1)
{
    int close = 9999;
    int dir = edt.gradient(loc, &close);
    int score = close <= breakpoint ? close : (close * mul / div);
    return Move(loc, dir, score, close);
}

Move Bot::pickMove(const Location &loc) const
{
    Move::score_queue pick;
    pick.push(makeMove(loc, e_food, 10, 3, 2));
    pick.push(makeMove(loc, e_explore));
    pick.push(makeMove(loc, e_attack, 5, 2, 3));
    pick.push(makeMove(loc, e_defend, 4, 2, 1));
    return pick.top();
}

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;

    queue<Location> frontier = visited_frontier();
    e_explore.update(frontier);

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
    for (State::iterator it = state.food.begin(); it != state.food.end(); ++it)
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

    Move::close_queue retry;
    bool moved = false;
    int angle = 0;
    int avoid = moves.size() < 5 ? (state.attackradius + 2) : 0;
    static const int *rotate[] = { AHEAD, RIGHT, LEFT };
    while (!moves.empty()) {
        const Move &move = moves.top();
        int dir = rotate[angle][move.dir];
        Location new_loc = state.getLocation(move.loc, dir);
        //if (!busy(new_loc) && e_food(new_loc) != 9999 && e_enemies(new_loc) > avoid) {
        if (!busy(new_loc) && e_food(new_loc) != 9999) {
            state.bug << "move " << move.loc << ": " << CDIRECTIONS[dir] << endl;
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
