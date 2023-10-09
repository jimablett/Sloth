#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "position.h"
#include "types.h"
#include "movegen.h"

namespace Sloth {

	namespace Search {

		extern HASHE hashTable[HASH_SIZE];

		extern U64 repetitionTable[1000];
		extern int repetitionIndex;
		
		extern int ply;

		void clearHashTable();
		//extern inline void writeHashEntry(int score, int depth, int hashFlag, Position& pos);
		//extern inline int readHashEntry(int alpha, int beta, int depth, Position& pos);

		void printMoveScores(Movegen::MoveList* moveList, Position& pos);

		extern inline int scoreMove(int move, Position& pos);
		extern inline int sortMoves(Movegen::MoveList* moveList, Position& pos);
		
		extern inline int negamax(int alpha, int beta, int depth, Position& pos);

		void search(Position& pos, int depth);
	}
}
#endif