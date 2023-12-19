#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "position.h"
#include "types.h"
#include "movegen.h"

namespace Sloth {

	namespace Search {

		struct PVariation {
			int length, score;
			int line[MAX_PLY];
		};

		extern int hashEntries;

		extern HASHE *hashTable;

		extern U64 repetitionTable[1000];
		extern int repetitionIndex;
		
		extern int ply;

		extern int bestMove;

		void clearHashTable();
		void initHashTable(int mb);
		//extern inline void writeHashEntry(int score, int depth, int hashFlag, Position& pos);
		//extern inline int readHashEntry(int alpha, int beta, int depth, Position& pos);

		void printMoveScores(Movegen::MoveList* moveList, Position& pos);

		extern inline int scoreMove(int move, Position& pos);
		extern inline int sortMoves(Movegen::MoveList* moveList, int bestMove, Position& pos);
		
		//extern inline int staticExchangeEvaluation(int move, Position& pos);

		extern inline int negamax(int alpha, int beta, int depth, Position& pos);

		//extern inline int aspirationSearch(int alpha, int beta, int depth, Position& pos);

		void search(Position& pos, int depth);
	}
}
#endif