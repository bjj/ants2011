#include <stdlib.h>

#include "Bot.h"

#include "edt.h"

using namespace std;

//constructor
Bot::Bot()
    : edt(state)
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

//makes the bots moves for the turn
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;
    state.bug << state << endl;

    queue<Location> food;
    for (State::iterator it = state.food.begin(); it != state.food.end(); ++it) {
        food.push(*it);
    }
    if (food.empty()) {
        for (State::iterator it = interesting.begin(); it != interesting.end(); ++it) {
            food.push(*it);
        }
    }
    if (state.myAnts.size() > 15) {
        for (State::iterator it = state.enemyHills.begin(); it != state.enemyHills.end(); ++it) {
            food.push(*it);
        }
    }
    edt.update(food);

    busy.reset();
    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        busy(state.myAnts[ant]) = true;
    }
    for(int ant=0; ant<(int)state.myAnts.size(); ant++) {
        state.bug << "ant " << ant << ": ";
        int my_dist = edt(state.myAnts[ant]);
        int best = -1;
        for(int d = 0; d<TDIRECTIONS; d++) {
            Location loc = state.getLocation(state.myAnts[ant], d);

            if(!busy(loc) && edt(loc) <= my_dist) {
                my_dist = edt(loc);
                best = d;
            }
        }
        if (best >= 0) {
            state.bug << "new move " << best << endl;
            state.makeMove(state.myAnts[ant], best);
            Location loc = state.getLocation(state.myAnts[ant], best);
            busy(state.myAnts[ant]) = false;
            busy(loc) = true;
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
