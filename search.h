#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "position.h"
#include "types.h"
#include "movegen.h"

namespace Sloth {

    namespace Search {

        extern int hashEntries;

        extern HASHE *hashTable;

        extern U64 repetitionTable[1000];
        extern int repetitionIndex;

        extern int ply;

        extern int bestMove;

        extern int contempt;

        void clearHashTable();
        void initHashTable(int mb);

        void printMoveScores(Movegen::MoveList* moveList, Position& pos);

        extern inline int scoreMove(int move, Position& pos);
        extern inline int sortMoves(Movegen::MoveList* moveList, int bestMove, Position& pos);

        extern inline int negamax(int alpha, int beta, int depth, bool cutnode, Position& pos);

        void search(Position& pos, int depth);
    }
}
#endif