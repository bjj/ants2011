#include "State.h"

#include <sstream>

using namespace std;

//constructor
State::State()
    : rows(200), cols(200) // for grid
{
    rows = cols = 0;
    gameover = 0;
    turn = 0;
    noPlayers = noHills = 0;
    diedByPlayer.resize(10, 0);
    ateByPlayer.resize(10, 0);
    avgHillSpacing = 0;
    stringstream ss;
    ss << "debug." << getpid() << ".txt";
    bug.open(ss.str());
}

//deconstructor
State::~State()
{
    bug.close();
}

//sets the state up
void State::setup()
{
    _row.init(rows);
    _col.init(cols);
    GridBase::rows = rows;
    GridBase::cols = cols;
    visionNeighborhood = neighborhood_offsets(viewradius);
    combatNeighborhood = dialate_neighborhood(neighborhood_offsets(attackradius), 2);
    for (int d = 0; d < TDIRECTIONS + 1; ++d)
        visionArc.push_back(neighborhood_arc(viewradius, d));
}

//resets all non-water squares to land and clears the bots ant vector
void State::reset()
{
    myAnts.clear();
    enemyAnts.clear();
    myHills.clear();
    enemyHills.clear();
    food.clear();
    for(int row=0; row<rows; row++)
        for(int col=0; col<cols; col++)
            grid[row][col].reset();
    fill(diedByPlayer.begin(), diedByPlayer.end(), 0);
    fill(ateByPlayer.begin(), ateByPlayer.end(), 0);
}

//outputs move information to the engine
void State::makeMove(const Location &loc, int direction)
{
    cout << "o " << loc.row << " " << loc.col << " " << CDIRECTIONS[direction] << endl;

    Location nLoc = getLocation(loc, direction);
    grid[nLoc.row][nLoc.col].ant = grid[loc.row][loc.col].ant;
    grid[loc.row][loc.col].ant = -1;
}

//returns the euclidean distance between two locations with the edges wrapped
double State::distance(const Location &loc1, const Location &loc2) const
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return sqrt(dr*dr + dc*dc);
}

/*
    This function will update update the lastSeen value for any squares currently
    visible by one of your live ants.

    BE VERY CAREFUL IF YOU ARE GOING TO TRY AND MAKE THIS FUNCTION MORE EFFICIENT,
    THE OBVIOUS WAY OF TRYING TO IMPROVE IT BREAKS USING THE EUCLIDEAN METRIC, FOR
    A CORRECT MORE EFFICIENT IMPLEMENTATION, TAKE A LOOK AT THE GET_VISION FUNCTION
    IN ANTS.PY ON THE CONTESTS GITHUB PAGE.
*/
void State::updateVisionInformation()
{
    std::queue<Location> locQueue;
    Location sLoc, cLoc, nLoc;

    visibleSquares = 0;
    for (iterator it = myAnts.begin(); it != myAnts.end(); ++it) {
        for (iterator vt = visionNeighborhood.begin(); vt != visionNeighborhood.end(); ++vt) {
            if (grid(deltaLocation(*it, (*vt).row, (*vt).col)).setVisible(turn))
                ++visibleSquares;
        }
    }

    for (LocationSet::iterator it = allEnemyHills.begin(); it != allEnemyHills.end();) {
        const Square &square = grid[(*it).row][(*it).col];
        if (square.isVisible && !square.isHill)
            allEnemyHills.erase(it++);
        else
            ++it;
    }
    for (LocationSet::iterator it = allMyHills.begin(); it != allMyHills.end();) {
        const Square &square = grid[(*it).row][(*it).col];
        if (square.isVisible && !square.isHill)
            allMyHills.erase(it++);
        else
            ++it;
    }
    for (LocationSet::iterator it = allFood.begin(); it != allFood.end();) {
        const Square &square = grid[(*it).row][(*it).col];
        if (square.isVisible && !square.isFood) {
            int eater = -1;
            for (int d = 0; d < TDIRECTIONS; ++d) {
                int player = grid(getLocation(*it, d)).ant;
                if (player == -1) {
                    continue;
                } else if (eater == -1) {
                    eater = player;
                } else if (eater != player) {
                    eater = -1;
                    break;
                }
            }
            if (eater >= 0)
                ateByPlayer[eater]++;
            allFood.erase(it++);
        } else
            ++it;
    }
}

/*
    This is the output function for a state. It will add a char map
    representation of the state to the output stream passed to it.

    For example, you might call "cout << state << endl;"
*/
ostream& operator<<(ostream &os, const State &state)
{
    for(int row=0; row<state.rows; row++)
    {
        for(int col=0; col<state.cols; col++)
        {
            if(state.grid[row][col].isWater)
                os << '%';
            else if(state.grid[row][col].isFood)
                os << '*';
            else if(state.grid[row][col].isHill)
                os << (char)('A' + state.grid[row][col].hillPlayer);
            else if(state.grid[row][col].ant >= 0)
                os << (char)('a' + state.grid[row][col].ant);
            else if(state.grid[row][col].isVisible)
                os << '.';
            else if(state.grid[row][col].wasVisible)
                os << ',';
            else
                os << '?';
        }
        os << endl;
    }

    return os;
}

//input function
istream& operator>>(istream &is, State &state)
{
    int row, col, player;
    string inputType, junk;

    //finds out which turn it is
    while(is >> inputType)
    {
        if(inputType == "end")
        {
            state.gameover = 1;
            break;
        }
        else if(inputType == "turn")
        {
            is >> state.turn;
            break;
        }
        else //unknown line
            getline(is, junk);
    }

    if(state.turn == 0)
    {
        //reads game parameters
        while(is >> inputType)
        {
            if(inputType == "loadtime")
                is >> state.loadtime;
            else if(inputType == "turntime")
                is >> state.turntime;
            else if(inputType == "rows")
                is >> state.rows;
            else if(inputType == "cols")
                is >> state.cols;
            else if(inputType == "turns")
                is >> state.turns;
            else if(inputType == "player_seed")
                is >> state.seed;
            else if(inputType == "players") //player information
                is >> state.noPlayers;
            else if(inputType == "viewradius2")
            {
                is >> state.viewradius;
                state.viewradius = sqrt(state.viewradius);
            }
            else if(inputType == "attackradius2")
            {
                is >> state.attackradius;
                state.attackradius = sqrt(state.attackradius);
            }
            else if(inputType == "spawnradius2")
            {
                is >> state.spawnradius;
                state.spawnradius = sqrt(state.spawnradius);
            }
            else if(inputType == "ready") //end of parameter input
            {
                state.timer.start();
                break;
            }
            else    //unknown line
                getline(is, junk);
        }
    }
    else
    {
        //reads information about the current turn
        while(is >> inputType)
        {
            if(inputType == "w") //water square
            {
                is >> row >> col;
                state.grid[row][col].isWater = 1;
                for (int d = 0; d < TDIRECTIONS; ++d)
                    state.grid(state.getLocation(Location(row,col), d)).byWater++;
            }
            else if(inputType == "f") //food square
            {
                is >> row >> col;
                state.grid[row][col].isFood = 1;
                state.food.push_back(Location(row, col));
                state.allFood.insert(Location(row, col));
            }
            else if(inputType == "a") //live ant square
            {
                is >> row >> col >> player;
                state.grid[row][col].ant = player;
                if(player == 0)
                    state.myAnts.push_back(Location(row, col));
                else
                    state.enemyAnts.push_back(Location(row, col));
            }
            else if(inputType == "d") //dead ant square
            {
                is >> row >> col >> player;
                state.diedByPlayer[player]++;
                state.grid[row][col].putDeadAnt(player);
            }
            else if(inputType == "h")
            {
                is >> row >> col >> player;
                state.grid[row][col].isHill = 1;
                state.grid[row][col].hillPlayer = player;
                if(player == 0) {
                    state.myHills.push_back(Location(row, col));
                    state.allMyHills.insert(Location(row, col));
                } else {
                    state.enemyHills.push_back(Location(row, col));
                    state.allEnemyHills.insert(Location(row, col));
                }

            }
            else if(inputType == "scores") //score information
            {
                state.scores = vector<double>(state.noPlayers, 0.0);
                for(int p=0; p<state.noPlayers; p++)
                    is >> state.scores[p];
            }
            else if(inputType == "go") //end of turn input
            {
                if(state.gameover)
                    is.setstate(std::ios::failbit);
                else
                    state.timer.start();
                break;
            }
            else {//unknown line
                state.bug << "unknown " << inputType << endl;
                getline(is, junk);
            }
        }
    }

    return is;
}

// NB: not quite the same as ants.py.  Includes (0,0), does not
// do offsetting of negative
vector<Location>
State::neighborhood_offsets(double max_dist) const
{
    vector<Location> neighborhood;
    int d = max_dist + 1;

    for (int dr = -d; dr <= d; ++dr) {
        for (int dc = -d; dc <= d; ++dc) {
            if (sqrt(dr*dr + dc*dc) <= max_dist)
                neighborhood.push_back(Location(dr, dc));
        }
    }
    return neighborhood;
}

vector<Location>
State::dialate_neighborhood(const vector<Location> &orig, int n) const
{
    LocationSet neighborhood(orig.begin(), orig.end());
    while (n--) {
        LocationSet dialated;
        for (LocationSet::iterator it = neighborhood.begin(); it != neighborhood.end(); ++it) {
            for (int d = 0; d < TDIRECTIONS + 1; ++d) {
                dialated.insert(Location((*it).row + DIRECTIONS[d][0], 
                                         (*it).col + DIRECTIONS[d][1]));
            }
        }
        swap(dialated, neighborhood);
    }
    return vector<Location>(neighborhood.begin(), neighborhood.end());
}

vector<Location>
State::neighborhood_arc(double max_dist, int d) const
{
    vector<Location> neighborhood = neighborhood_offsets(max_dist);
    vector<Location> arc;

    for (State::iterator it = neighborhood.begin(); it != neighborhood.end(); ++it) {
        Location test((*it).row + DIRECTIONS[d][0], 
                      (*it).col + DIRECTIONS[d][1]);
        if (find(neighborhood.begin(), neighborhood.end(), test) == neighborhood.end())
            arc.push_back(*it);
    }
    return arc;
}
