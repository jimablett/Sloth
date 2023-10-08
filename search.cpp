#include "search.h"

#include <iostream>

#include "evaluate.h"
#include "movegen.h"

namespace Sloth {
	//HASHE Search::hashTable[HASH_SIZE];

	HASHE Search::hashTable[HASH_SIZE];

	const int fullDepthMoves = 4;
	const int reductionLimit = 3;

	//int* pvLength = new int[MAX_PLY];
	int pvLength[MAX_PLY];
	int pvTable[MAX_PLY][MAX_PLY];

	int followPV, scorePV;

	int ply;

	long nodes;

	int killerMoves[2][MAX_PLY]; // id, ply
	int historyMoves[12][64]; // piece, square

	void Search::clearHashTable() { // clears the transposition (hash) table
		for (int i = 0; i < HASH_SIZE; i++) {
			hashTable[i].hashKey = 0;
			hashTable[i].depth = 0;
			hashTable[i].flag = 0;
			hashTable[i].score = 0;
		}
		//memset(hashTable, 0, HASH_SIZE * sizeof(HASHE)); // does this work??
	}

	static inline int readHashEntry(int alpha, int beta, int depth, Position& pos) {
		HASHE* hashEntry = &Search::hashTable[pos.hashKey % HASH_SIZE]; // pointer to hash entry

		if (hashEntry->hashKey == pos.hashKey) { // make sure that we are dealing with the exact position that we need
			if (hashEntry->depth >= depth) {

				// matching PV node score
				if (hashEntry->flag == hashfEXACT) {
					return hashEntry->score;
				}

				// match alpha score
				if ((hashEntry->flag == hashfALPHA) && (hashEntry->score <= alpha)) {
					return alpha;
				}

				// match beta score
				if ((hashEntry->flag == hashfBETA) && (hashEntry->score >= beta)) {
					return beta;
				}
			}
		}

		return NO_HASH_ENTRY; // hash entry doesnt exist
	}

	static inline void writeHashEntry(int score, int depth, int hashFlag, Position& pos) {
		HASHE* hashEntry = &Search::hashTable[pos.hashKey % HASH_SIZE];

		hashEntry->hashKey = pos.hashKey;
		hashEntry->score = score;
		hashEntry->flag = hashFlag;
		hashEntry->depth = depth;
	}

	static inline void enablePVScoring(Movegen::MoveList* movelist) {
		followPV = 0;

		for (int i = 0; i < movelist->count; i++) {
			if (pvTable[0][ply] == movelist->moves[i]) {
				scorePV = 1;
				followPV = 1;
			}
		}
	}

	void Search::printMoveScores(Movegen::MoveList* moveList, Position& pos) {
		for (int i = 0; i < moveList->count; i++) {
			int move = moveList->moves[i];

			Movegen::printMove(move);

			printf("score: %d \n", Search::scoreMove(move, pos));

			//Search::scoreMove(move, pos);
		}
	}

	/*
		How moves are ordered
		1. PV move
		2. Captures in MVV/LVA
		3. 1st killer move
		4. 2nd killer move
		5. History moves
		6. Unsorted moves
	*/

	inline int Search::scoreMove(int move, Position& pos) {
		if (scorePV) {
			if (pvTable[0][ply] == move) {
				scorePV = 0;

				// give PV move the highest score
				return 20000;
			}
		}

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
			}
			else if (killerMoves[1][ply] == move) {
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

					moveScores[cur] = moveScores[next];
					moveScores[next] = tmpScore;

					int tmpMove = moveList->moves[cur];

					moveList->moves[cur] = moveList->moves[next];
					moveList->moves[next] = tmpMove;
				}
			}
		}

		delete[] moveScores; // free the memory

		return 0;
	}

	static inline int quiescence(int alpha, int beta, Position& pos) {

		if ((nodes & 2047) == 0) pos.time.communicate(); // listen to gui/user input every 2047 nodes

		nodes++;

		if (ply > MAX_PLY - 1) return Eval::evaluate(pos);

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

			if (pos.time.stopped == true) return 0;

			// if better move is found
			if (score > alpha) {
				alpha = score; //PV node

				// using fail-hard beta cutoff
				if (score >= beta) {
					return beta;
				}
			}

		}

		return alpha;
	}

	inline int Search::negamax(int alpha, int beta, int depth, Position& pos) {
		int score = 0;

		int hashFlag = hashfALPHA;

		if (ply < 0) ply = 0; // this wasnt a part of the plan but without it ply gets set to gazillions for some reason

		if (ply && (score = readHashEntry(alpha, beta, depth, pos)) != NO_HASH_ENTRY) { // if move has already been searched then return score immediately
			return score;
		}


		if ((nodes & 2047) == 0) pos.time.communicate();

		pvLength[ply] = ply; // inits the PV length

		//printf("\n%d\n", ply);

		// recursion escape condition
		if (depth == 0) return quiescence(alpha, beta, pos);

		// preventing overflow of arrays
		if (ply > MAX_PLY - 1) return Eval::evaluate(pos);

		nodes++;

		int kingCheck = pos.isSquareAttacked((pos.sideToMove == Colors::white) ? Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::K]) : Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::k]), pos.sideToMove ^ 1);

		if (kingCheck) depth++; // If the king is in check, then we increase ply depth by one to prevent immediately getting mated

		int legalMoves = 0;

		// null move pruning
		if (depth >= 3 && kingCheck == 0 && ply) {
			copyBoard(pos);

			ply++;

			if (pos.enPassant != no_sq) // hash enpassant if available
				pos.hashKey ^= Zobrist::enPassantKeys[pos.enPassant];

			pos.enPassant = no_sq;

			pos.sideToMove ^= 1; // switching the side gives the opponent an extra move to make

			pos.hashKey ^= Zobrist::sideKey;

			score = -negamax(-beta, -beta + 1, depth - 1 - 2, pos); // search with reduced depth to find beta cutoffs (2 is the reduction limit)

			ply--;

			takeBack(pos);

			if (pos.time.stopped == true) return 0; // returns 0 if time is up

			// fail hard beta cutoff
			if (score >= beta) return beta;
		}

		// At this point we are creating many movelists (Around the whole chess engine), so for later, consider generating every legal after a move has been played, so that we dont have to generate over and over again, in the same position
		Movegen::MoveList moveList[1];

		Movegen::generateMoves(pos, moveList);

		if (followPV) {
			enablePVScoring(moveList);
		}

		sortMoves(moveList, pos); // sort the moves to speed stuff up (doing the same in quiescence search)

		int movesSearched = 0;

		for (int c = 0; c < moveList->count; c++) {
			copyBoard(pos);

			ply++;

			if (pos.makeMove(pos, moveList->moves[c], allMoves) == 0) { // make sure to only make the legal moves
				ply--;

				continue; // skip to next move
			}

			legalMoves++;

			if (movesSearched == 0) {
				score = -negamax(-beta, -alpha, depth - 1, pos); // doing the normal AB search
			}
			else { // late move reduction
				if (movesSearched >= fullDepthMoves && depth >= reductionLimit && kingCheck == 0 && !getMoveCapture(moveList->moves[c]) && !getMovePromotion(moveList->moves[c])) // not having getMoveCapture and promotion was faster
					score = -negamax(-alpha - 1, -alpha, depth - 2, pos);
				else
					score = alpha + 1;

				// principle variation search
				if (score > alpha) {
					score = -negamax(-alpha - 1, -alpha, depth - 1, pos); // better move has been found during LMR, re-search at full depth but with narrowed score bandwith

					// if fails to prove that other moves are bad
					if ((score > alpha) && (score < beta)) { // if LMR fails, re-search at full depth and full score bandwith
						score = -negamax(-beta, -alpha, depth - 1, pos);
					}
				}
			}

			ply--;

			takeBack(pos);

			if (pos.time.stopped == true) return 0;

			movesSearched++;

			// if better move is found
			if (score > alpha) {
				// switch hash flag
				hashFlag = hashfEXACT;

				if (getMoveCapture(moveList->moves[c]) == 0) // this CAN be removed
					historyMoves[getMovePiece(moveList->moves[c])][getMoveTarget(moveList->moves[c])] += depth;

				alpha = score; //PV node

				pvTable[ply][ply] = moveList->moves[c];

				for (int next = ply + 1; next < pvLength[ply + 1]; next++) {
					pvTable[ply][next] = pvTable[ply + 1][next];
				}

				pvLength[ply] = pvLength[ply + 1];

				// using fail-hard beta cutoff
				if (score >= beta) {
					// store hash entry
					writeHashEntry(beta, depth, hashfBETA, pos);

					if (getMoveCapture(moveList->moves[c]) == 0) {
						killerMoves[1][ply] = killerMoves[0][ply];
						killerMoves[0][ply] = moveList->moves[c];
					}

					return beta;
				}
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

		writeHashEntry(alpha, depth, hashFlag, pos);

		return alpha; // move fails low
	}
	
	void Search::search(Position& pos, int depth) {
		int score = 0;

		// clear out garbage
		nodes = 0;

		pos.time.stopped = false;

		followPV = 0;
		scorePV = 0;

		memset(killerMoves, 0, sizeof(killerMoves));
		memset(historyMoves, 0, sizeof(historyMoves));
		memset(pvTable, 0, sizeof(pvTable));
		memset(pvLength, 0, sizeof(pvLength)); // is this really needed?

		int alpha = -50000;
		int beta = 50000;

		// iterative deepening
		for (int curDepth = 1; curDepth <= depth; curDepth++) {
			if (pos.time.stopped) break; // if time is up then stop calculating and return best move so far

			followPV = 1;

			score = negamax(alpha, beta, curDepth, pos);

			if ((score <= alpha) || (score >= beta)) {
				alpha = -50000;
				beta = 50000;
				continue;
			}

			alpha = score - 50;
			beta = score + 50;

			printf("info score cp %d depth %d nodes %ld time %d pv ", score, curDepth, nodes, pos.time.getTimeMs() - pos.time.startTime);

			for (int c = 0; c < pvLength[0]; c++) {
				Movegen::printMove(pvTable[0][c]);
				printf(" ");
			}

			printf("\n");
		}


		printf("bestmove ");
		Movegen::printMove(pvTable[0][0]); // first element within PV table
		printf("\n");
	}
}