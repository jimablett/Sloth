#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "position.h"
#include "movegen.h"

namespace Sloth {
	namespace Search {
		void printMoveScores(Movegen::MoveList* moveList, Position& pos);

		extern inline int scoreMove(int move, Position& pos);
		extern inline int sortMoves(Movegen::MoveList* moveList, Position& pos);
		
		extern inline int negamax(int alpha, int beta, int depth, Position& pos);

		void search(Position& pos, int depth);
	}
}
#endif