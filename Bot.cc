#include <stdlib.h>

#include "Bot.h"

#include "edt.h"

using namespace std;

//constructor
Bot::Bot()
    : e_food(state)
    , e_explore(state)
    , e_attack(state)
    , e_defend(state)
    , busy(state)
{
    srandom(time(0));
}

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;
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

Move Bot::pickMove(const Location &loc) const
{
    priority_queue<Move> pick;
    pick.push(Move(loc, e_food.gradient(loc), e_food(loc)));
    pick.push(Move(loc, e_explore.gradient(loc), e_explore(loc)));
    pick.push(Move(loc, e_attack.gradient(loc), e_attack(loc) / 4));
    return pick.top();
}

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;

    queue<Location> frontier = visited_frontier();
    e_explore.update(frontier);

    state.bug << state << endl;

    queue<Location> food;
    for (State::iterator it = state.food.begin(); it != state.food.end(); ++it)
        food.push(*it);
    e_food.update(food);

    queue<Location> victims;
    if (state.myAnts.size() > 15) {
        for (set<Location>::iterator it = state.allEnemyHills.begin(); it != state.allEnemyHills.end(); ++it)
            victims.push(*it);
    }
    e_attack.update(victims);

    priority_queue<Move> moves;

    set<Location> defense;
    if (state.myAnts.size() > 15) {
        for (State::iterator it = state.myHills.begin(); it != state.myHills.end(); ++it) {
            const Square &hill = state.grid[(*it).row][(*it).col];
            if (!hill.isVisible)
                continue;

            int empty_dir = -1;
            for (int d = 0; d < TDIRECTIONS; ++d) {
                const Location loc = state.getLocation(*it, d);
                const Square &square = state.grid[loc.row][loc.col];
                if (!square.isWater && square.ant != 0) {
                    empty_dir = d;
                    break;
                }
            }

            bool need_escape = hill.ant == 0;

            for (int d = 0; d < TDIRECTIONS; ++d) {
                const Location loc = state.getLocation(*it, d);
                if (need_escape) {
                    if (empty_dir != -1 ? (d == empty_dir) : (pickMove(loc).dir != -1)) {
                        // handle hill here:
                        defense.insert(*it);
                        moves.push(Move(*it, d, 999));
                        need_escape = false;
                        continue;
                    }
                }
                defense.insert(loc);
            }
        }
    }

    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        const Location & loc = state.myAnts[ant];

        if (defense.count(loc))
            continue;

        const Move m = pickMove(loc);

        state.bug << "ant " << ant << ": " << m.dir << " (" << m.close << ")" << endl;

        if (m.dir >= 0)
            moves.push(m);
    }

    busy.reset();
    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        busy(state.myAnts[ant]) = true;
    }
    while (!moves.empty()) {
        const Move &move = moves.top();
        Location new_loc = state.getLocation(move.loc, move.dir);
        if (!busy(new_loc)) {
            state.bug << "new move " << CDIRECTIONS[move.dir] << endl;
            state.makeMove(move.loc, move.dir);
            busy(move.loc) = false;
            busy(new_loc) = true;
        }
        moves.pop();
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
