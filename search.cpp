#include "search.h"

#include <iostream>

#include "evaluate.h"
#include "movegen.h"

namespace Sloth {
	int pvLength[64];
	int pvTable[64][64];

	int ply;

	long nodes;

	int killerMoves[2][64]; // id, ply
	int historyMoves[12][64]; // piece, square

	void Search::printMoveScores(Movegen::MoveList* moveList, Position& pos) {
		for (int i = 0; i < moveList->count; i++) {
			int move = moveList->moves[i];

			Movegen::printMove(move);

			printf("score: %d \n", Search::scoreMove(move, pos));

			//Search::scoreMove(move, pos);
		}
	}

	inline int Search::scoreMove(int move, Position& pos) {
		if (getMoveCapture(move)) {
			int targetPiece = Piece::P;

			int startPiece, endPiece;

			startPiece = (pos.sideToMove == Colors::white) ? Piece::p : Piece::P;
			endPiece = (pos.sideToMove == Colors::white) ? Piece::k : Piece::K;

			for (int bbPiece = startPiece; bbPiece <= endPiece; bbPiece++) {
				if (getBit(Bitboards::bitboards[bbPiece], getMoveTarget(move))) {
					targetPiece = bbPiece;

					break;
				}
			}

			return MVV_LVA[getMovePiece(move)][targetPiece] + 10000;
		}
		else { // scoring quiet move
			if (killerMoves[0][ply] == move) {
				return 9000;
			} else if (killerMoves[1][ply] == move) {
				return 8000;
			}
			else {
				return historyMoves[getMovePiece(move)][getMoveTarget(move)];
			}
		}

		return 0;
	}


	/*
	* COMMENT: try this
	just a side note, at least in my engine, I have found it is more efficient to do a similar selection sort, but instead of sorting the list all at once, we select the best move, play it, and then repeat. It's better to do it this way because we don't spend time sorting moves the engine decides to prune anyway.
	*/
	inline int Search::sortMoves(Movegen::MoveList* moveList, Position& pos) {
		int* moveScores = new int[moveList->count];

		for (int i = 0; i < moveList->count; i++) {
			moveScores[i] = scoreMove(moveList->moves[i], pos);
		}

		// might be a faster way to do this??
		for (int cur = 0; cur < moveList->count; cur++) {
			for (int next = cur + 1; next < moveList->count; next++) { // loops over the next move
				if (moveScores[cur] < moveScores[next]) { //if score of cur move is worse than the score of the next move then swap the scores
					int tmpScore = moveScores[cur];
					int tmpMove = moveList->moves[cur];

					moveScores[cur] = moveScores[next];
					moveScores[next] = tmpScore;

					moveList->moves[cur] = moveList->moves[next];
					moveList->moves[next] = tmpMove;
				}
			}
		}

		delete[] moveScores; // free the memory

		return 0;
	}

	static inline int quiescence(int alpha, int beta, Position& pos) {
		nodes++;

		int eval = Eval::evaluate(pos);

		// using fail-hard beta cutoff
		if (eval >= beta) {
			return beta;
		}

		// if better move is found
		if (eval > alpha) {
			alpha = eval; //PV node
		}

		Movegen::MoveList moveList[1];

		Movegen::generateMoves(pos, moveList);

		Search::sortMoves(moveList, pos);

		for (int c = 0; c < moveList->count; c++) {
			copyBoard(pos);

			ply++;

			if (pos.makeMove(pos, moveList->moves[c], captures) == 0) { // make sure to only make the legal moves
				ply--;

				continue; // skip to next move
			}

			int score = -quiescence(-beta, -alpha, pos);

			ply--;

			takeBack(pos);

			// using fail-hard beta cutoff
			if (score >= beta) {
				return beta;
			}

			// if better move is found
			if (score > alpha) {
				alpha = score; //PV node
			}
		}

		return alpha;
	}

	inline int Search::negamax(int alpha, int beta, int depth, Position& pos) {
		
		pvLength[ply] = ply; // inits the PV length

		// recursion escape condition
		//if (depth == 0) return Eval::evaluate(pos);
		if (depth == 0) return quiescence(alpha, beta, pos);

		// preventing overflow of arrays
		if (ply > MAX_PLY - 1) return Eval::evaluate(pos);

		nodes++;

		int kingCheck = pos.isSquareAttacked((pos.sideToMove == Colors::white) ? Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::K]) : Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::k]), pos.sideToMove ^ 1);
		
		if (kingCheck) depth++; // If the king is in check, then we increase ply depth by one to prevent immediately getting mated
		
		int legalMoves = 0;

		// At this point we are creating many movelists (Around the whole chess engine), so for later, consider generating every legal after a move has been played, so that we dont have to generate over and over again, in the same position
		Movegen::MoveList moveList[1];
		
		Movegen::generateMoves(pos, moveList);

		sortMoves(moveList, pos); // sort the moves to speed stuff up (doing the same in quiescence search)

		for (int c = 0; c < moveList->count; c++) {
			copyBoard(pos);

			ply++;

			if (pos.makeMove(pos, moveList->moves[c], allMoves) == 0) { // make sure to only make the legal moves
				ply--;

				continue; // skip to next move
			}

			legalMoves++;

			int score = -negamax(-beta, -alpha, depth - 1, pos);

			ply--;

			takeBack(pos);

			// using fail-hard beta cutoff
			if (score >= beta) {
				if (getMoveCapture(moveList->moves[c]) == 0) {
					killerMoves[1][ply] = killerMoves[0][ply];
					killerMoves[0][ply] = moveList->moves[c];
				}

				return beta;
			}

			// if better move is found
			if (score > alpha) {
				if (getMoveCapture(moveList->moves[c]) == 0) // this CAN be removed
					historyMoves[getMovePiece(moveList->moves[c])][getMoveTarget(moveList->moves[c])] += depth;

				alpha = score; //PV node

				pvTable[ply][ply] = moveList->moves[c];

				for (int next = ply + 1; next < pvLength[ply + 1]; next++) {
					pvTable[ply][next] = pvTable[ply + 1][next];
				}

				pvLength[ply] = pvLength[ply + 1];
			}
		}

		if (legalMoves == 0) {
			if (kingCheck) {
				return -49000 + ply;
			}
			else {
				// stalemate score
				return 0;
			}
		}

		return alpha; // move fails low
	}
	
	void Search::search(Position& pos, int depth) {
		// find the best move (32001 infinite value in stockfish)
		//int score = negamax(-50000, 50000, depth, pos);

		// clear out garbage
		nodes = 0;

		memset(killerMoves, 0, sizeof(killerMoves));
		memset(historyMoves, 0, sizeof(historyMoves));
		memset(pvTable, 0, sizeof(pvTable));
		memset(pvLength, 0, sizeof(pvLength)); // is this really needed?

		// iterative deepening
		for (int curDepth = 1; curDepth <= depth; curDepth++) {
			int score = negamax(-50000, 50000, curDepth, pos);

			printf("info score cp %d depth %d nodes %ld pv ", score, curDepth, nodes);

			for (int c = 0; c < pvLength[0]; c++) {
				Movegen::printMove(pvTable[0][c]);
				printf(" ");
			}

			printf("\n");
		}


		printf("bestmove ");
		Movegen::printMove(pvTable[0][0]); // first element within PV table
		printf("\n");

		//printf("\nmove val: %d, old best:\n", bestMove);
		//Movegen::printMove(oldBest);
		//printf("\n");

	}
}