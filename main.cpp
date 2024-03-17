/*
    Big thanks to:
    https://www.youtube.com/@chessprogramming591
    https://www.chessprogramming.org/

*/

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <string.h>

#include <chrono>

#include "uci.h"
#include "piece.h"
#include "magic.h"
#include "movegen.h"
#include "types.h"


// testing
#include "position.h"
#include "perft.h"
#include "uci.h"
#include "search.h"
#include "evaluate.h"

using namespace Sloth;

int main(int argc, char* argv[])
{
    //Position pos;

    Magic::initAttacks();
    Bitboards::initLeaperAttacks();
    Zobrist::initRandomKeys();
    Search::initHashTable(64);
    //TranspositionTable::initTT();
    Eval::initEvalMasks();
    //game.parseFen(startPosition);

    int debug = 0;

    if (debug) {
        Position pos;

        Movegen::MoveList movelist[1];

        pos.parseFen("8/8/8/8/8/8/PPPPPPPP/8 w - - 0 1");
       
        pos.printBoard();
        
    } else UCI::loop();

    free(Search::hashTable);

    return 0;
}