#include <iostream>
#include <algorithm>

#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "magic.h"

#include "uci.h"

namespace Sloth {

	int Search::hashEntries = 0;

	HASHE* Search::hashTable = NULL;

	U64 Search::repetitionTable[1000];
	int Search::repetitionIndex = 0;

	const int fullDepthMoves = 4;
	const int reductionLimit = 3;

	//int* pvLength = new int[MAX_PLY];
	int pvLength[MAX_PLY];
	int pvTable[MAX_PLY][MAX_PLY];

	int followPV, scorePV;

	int Search::ply = 0;
	int Search::contempt = 25;

	unsigned long long nodes;

	int killerMoves[2][MAX_PLY]; // id, Search::ply
	int historyMoves[12][64]; // piece, square

	int totalEntries = 0;
	int usedEntries = 0;

	const int lmpMargins[4] = { 0, 8, 12, 24 };
	const int pieceValues[13] = { 100, 300, 300, 500, 900, VALUE_INFINITE, 100, 300, 300, 500, 900, VALUE_INFINITE, 0 };

	void Search::clearHashTable() { // clears the transposition (hash) table
		HASHE* hashEntry;

		totalEntries = hashEntries;
		usedEntries = 0;

		//std::memset(Search::hashTable, 0, sizeof(Search::hashTable));

		for (hashEntry = hashTable; hashEntry < hashTable + hashEntries; hashEntry++) {
			hashEntry->hashKey = 0;
			hashEntry->depth = 0;
			hashEntry->flag = 0;
			hashEntry->score = 0;
		}
	}

	void Search::initHashTable(int mb) {
		int hashSize = 0x100000 * mb;

		hashEntries = hashSize / sizeof(HASHE);

		if (hashTable != NULL) {
			printf("info string Clearing hash memory\n");

			free(hashTable);
		}

		hashTable = (HASHE*)malloc(hashEntries * sizeof(HASHE));

		totalEntries = mb * 1024 * 1024 / sizeof(HASHE);
		usedEntries = 0;

		if (hashTable == NULL) {
			printf("info string Couldnt allocate memory for hash table, trying %dMB", mb / 2);

			initHashTable(mb / 2);
		}
		else {
			clearHashTable();

			printf("info string Hash table is initialized with %d entries\n", hashEntries);
		}
	}

	static inline int readHashEntry(int alpha, int beta, int* bestMove, int depth, Position& pos) {
		HASHE* hashEntry = &Search::hashTable[pos.hashKey % Search::hashEntries]; // pointer to hash entry

		if (hashEntry->hashKey == pos.hashKey) { // make sure that we are dealing with the exact position that we need
			if (hashEntry->depth >= depth) {

				int score = hashEntry->score;

				if (score < -MATE_SCORE) score += Search::ply;
				if (score > MATE_SCORE) score -= Search::ply;

				// matching PV node score
				if (hashEntry->flag == hashfEXACT) {
					return score;
				}

				// match alpha score
				if ((hashEntry->flag == hashfALPHA) && (score <= alpha)) {
					return alpha;
				}

				// match beta score
				if ((hashEntry->flag == hashfBETA) && (score >= beta)) {
					return beta;
				}
			}

			*bestMove = hashEntry->bestMove;
		}
		return NO_HASH_ENTRY; // hash entry doesnt exist
	}

	static inline void writeHashEntry(int score, int bestMove, int depth, int hashFlag, Position& pos) {
		HASHE* hashEntry = &Search::hashTable[pos.hashKey % Search::hashEntries];

		if (score < -MATE_SCORE) score -= Search::ply;
		if (score > MATE_SCORE) score += Search::ply;

		if (hashEntry->depth == 0)
			usedEntries++;

		hashEntry->hashKey = pos.hashKey;
		hashEntry->score = score;
		hashEntry->flag = hashFlag;
		hashEntry->depth = depth;
		hashEntry->bestMove = bestMove;
	}

	double hashFull() {
		return 1000 * usedEntries / totalEntries;
	}

	static inline void enablePVScoring(Movegen::MoveList* movelist) {
		followPV = 0;

		for (int i = 0; i < movelist->count; i++) {
			if (pvTable[0][Search::ply] == movelist->moves[i]) {
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
		int score = 0;

		if (scorePV) {
			if (pvTable[0][Search::ply] == move) {
				scorePV = 0;
				return 20000;  // give PV move the highest score
			}
		}

		if (getMoveCapture(move)) {
			int targetPiece = Piece::P;
			int startPiece = (pos.sideToMove == Colors::white) ? Piece::p : Piece::P;
			int endPiece = (pos.sideToMove == Colors::white) ? Piece::k : Piece::K;

			// Store the target square to avoid repeated calculations
			U64 targetSquare = getMoveTarget(move);

			for (int bbPiece = startPiece; bbPiece <= endPiece; bbPiece++) {
				U64 bb = Bitboards::bitboards[bbPiece];
				if (getBit(bb, targetSquare)) {
					targetPiece = bbPiece;
					break;
				}
			}

			score = MVV_LVA[getMovePiece(move)][targetPiece] + 10000;
		}
		else {  // scoring quiet move
			int killerScore = 0;
			if (killerMoves[0][Search::ply] == move) {
				killerScore = 9000;
			}
			else if (killerMoves[1][Search::ply] == move) {
				killerScore = 8000;
			}

			score = (killerScore != 0) ? killerScore : historyMoves[getMovePiece(move)][getMoveTarget(move)];
		}

		return score;
	}

	inline int Search::sortMoves(Movegen::MoveList* moveList, int bestMove, Position& pos) {
		int* moveScores = new int[moveList->count];

		for (int i = 0; i < moveList->count; i++) {
			if (bestMove == moveList->moves[i]) {
				moveScores[i] = 30000;
			}
			else {
				moveScores[i] = scoreMove(moveList->moves[i], pos);
			}

			//moveScores[i] = scoreMove(moveList->moves[i], pos);
		}

		for (int i = 1; i < moveList->count; i++) { // faster solution
			int currentMove = moveList->moves[i];
			int currentScore = moveScores[i];
			int j = i - 1;

			while (j >= 0 && moveScores[j] < currentScore) {
				moveList->moves[j + 1] = moveList->moves[j];
				moveScores[j + 1] = moveScores[j];
				j--;
			}

			moveList->moves[j + 1] = currentMove;
			moveScores[j + 1] = currentScore;
		}

		delete[] moveScores; // free the memory

		return 0;
	}

	static inline int isRepetition(Position& pos) {
		for (int i = 0; i < Search::repetitionIndex; i++) {
			if (Search::repetitionTable[i] == pos.hashKey) {
				// repetition found
				return 1;
			}
		}

		return 0; // no repetitions
	}

	static inline bool isEndgame(Position& pos) {
		int pawnMaterial = Bitboards::countBits(Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) * 100;
		int knightMaterial = Bitboards::countBits(Bitboards::bitboards[Piece::N] | Bitboards::bitboards[Piece::n]) * 320;
		int bishopMaterial = Bitboards::countBits(Bitboards::bitboards[Piece::B] | Bitboards::bitboards[Piece::b]) * 320;
		int rookMaterial = Bitboards::countBits(Bitboards::bitboards[Piece::R] | Bitboards::bitboards[Piece::r]) * 500;
		int queenMaterial = Bitboards::countBits(Bitboards::bitboards[Piece::Q] | Bitboards::bitboards[Piece::q]) * 950;

		return ((pawnMaterial + knightMaterial + bishopMaterial + rookMaterial + queenMaterial) < 2600);
	}

	static inline int contemptFactor(Position& pos) {
		if (isEndgame(pos))
			return 0;
		else
			return pos.sideToMove == Colors::white ? -Search::contempt : Search::contempt;
	}

	static inline U64 considerXrays(int sq, U64 occ) {
		U64 attackers = 0ULL;

		U64 attackingBishops = Bitboards::bitboards[Piece::B] | Bitboards::bitboards[Piece::b];
		U64 attackingRooks = Bitboards::bitboards[Piece::R] | Bitboards::bitboards[Piece::r];
		U64 attackingQueens = Bitboards::bitboards[Piece::Q] | Bitboards::bitboards[Piece::q];

		U64 intercardinalRays = Magic::getBishopAttacks(sq, occ);
		U64 cardinalRays = Magic::getRookAttacks(sq, occ);

		attackers |= intercardinalRays & (attackingBishops | attackingQueens);
		attackers |= cardinalRays & (attackingRooks | attackingQueens);

		return attackers;
	}

	static inline U64 minAttacker(U64 attadef, int sideToMove, int& attacker) {
		int startPiece = Piece::P;
		int endPiece = Piece::K;

		if (sideToMove == Colors::black) {
			startPiece = Piece::p;
			endPiece = Piece::k;
		}

		for (attacker = startPiece; attacker <= endPiece; attacker++) {
			U64 subset = attadef & Bitboards::bitboards[attacker];

			if (subset) return (subset & (0 - subset));
		}

		return 0;
	}

	static inline int see(int move, Position& pos) {
		int gain[32];
		int idepth = 0;
		int sideToMove = pos.sideToMove ^ 1;

		int fromSq = getMoveSource(move);
		int toSq = getMoveTarget(move);
		int attacker = getMovePiece(move);

		int startPiece = Piece::P;
		int endPiece = Piece::K;
		int target = -1;

		if (sideToMove == Colors::black) {
			startPiece = Piece::p;
			endPiece = Piece::k;
		}

		for (int piece = startPiece; piece <= endPiece; piece++) {
			if (getBit(Bitboards::bitboards[piece], toSq)) {
				target = piece;

				break;
			}
		}

		if (target < 0) return 0;

		U64 seen = 0ULL;
		U64 occupied = Bitboards::occupancies[Colors::both];
		U64 attackerBB = 1ULL << fromSq;

		U64 attadef = pos.attackersTo(toSq, occupied);
		U64 maxXray = occupied & ~(Bitboards::bitboards[Piece::N] | Bitboards::bitboards[Piece::K] | Bitboards::bitboards[Piece::n] | Bitboards::bitboards[Piece::k]);

		gain[idepth] = pieceValues[target];

		while (attackerBB) {
			idepth++;
			gain[idepth] = pieceValues[attacker] - gain[idepth - 1];

			if (std::max(-gain[idepth - 1], gain[idepth]) < 0) {
				break;
			}

			attadef &= ~attackerBB;
			occupied &= ~attackerBB;
			seen |= attackerBB;

			if ((attackerBB & maxXray) != 0) {
				attadef = considerXrays(toSq, occupied) & ~seen;
			}

			attackerBB = minAttacker(attadef, sideToMove, attacker);
			sideToMove ^= 1;
		}

		for (idepth--; idepth > 0; idepth--) {
			gain[idepth - 1] = -std::max(-gain[idepth - 1], gain[idepth]);
		}

		return gain[0];
	}

	static inline int quiescence(int alpha, int beta, Position& pos) {

		if ((nodes & 2047) == 0) pos.time.communicate(); // listen to gui/user input every 2047 nodes

		nodes++;

		if (Search::ply > MAX_PLY - 1) return Eval::evaluate(pos);

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

		Movegen::generateMoves(pos, moveList, true);

		Search::sortMoves(moveList, 0, pos);

		for (int c = 0; c < moveList->count; c++) {
			int staticExchangeEvaluation = see(moveList->moves[c], pos);

			if (staticExchangeEvaluation < 0) continue;

			if (eval + 97 <= alpha && staticExchangeEvaluation <= 0) continue;

			copyBoard(pos);

			Search::ply++;

			Search::repetitionIndex++;
			Search::repetitionTable[Search::repetitionIndex] = pos.hashKey;

			if (pos.makeMove(pos, moveList->moves[c], captures) == 0) { // make sure to only make the legal moves
				Search::ply--;

				Search::repetitionIndex--;

				continue; // skip to next move
			}

			int score = -quiescence(-beta, -alpha, pos);

			Search::ply--;
			Search::repetitionIndex--;

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

	inline int Search::negamax(int alpha, int beta, int depth, bool cutnode, Position& pos) {

		pvLength[Search::ply] = Search::ply; // inits the PV length

		int score = 0;
		int bestMove = 0;

		int hashFlag = hashfALPHA;

		bool pvNode = beta - alpha > 1;

		if (Search::ply && isRepetition(pos)) return 0; // draw score, repetition has occured

		if (Search::ply && (score = readHashEntry(alpha, beta, &bestMove, depth, pos)) != NO_HASH_ENTRY && !pvNode) { // if move has already been searched then return score immediately
			return score;
		}

		if ((nodes & 2047) == 0) pos.time.communicate();

		// recursion escape condition
		if (depth == 0) return quiescence(alpha, beta, pos);

		// preventing overflow of arrays
		if (Search::ply > MAX_PLY - 1) return Eval::evaluate(pos);

		nodes++;

		int kingCheck = pos.isSquareAttacked((pos.sideToMove == Colors::white) ? Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::K]) : Bitboards::getLs1bIndex(Bitboards::bitboards[Piece::k]), pos.sideToMove ^ 1);

		if (kingCheck) depth++; // If the king is in check, then we increase Search::ply depth by one to prevent immediately getting mated

		int legalMoves = 0;

		int staticEval = Eval::evaluate(pos);

		if (ply && !pvNode && depth < 2 && (staticEval + 339) <= alpha) return quiescence(alpha, beta, pos);

		if (depth < 3 && !pvNode && !kingCheck && abs(beta - 1) > -VALUE_INFINITE + 100) {
			int evalMargin = 120 * depth;

			if (staticEval - evalMargin >= beta) {
				return staticEval - evalMargin;
			}
		}

		if (staticEval - 80 * depth >= beta && !kingCheck && depth < 9 && !pvNode) {
			return staticEval;
		}

		// null move pruning
		if (depth >= 3 && kingCheck == 0 && Search::ply) {
			copyBoard(pos);

			Search::ply++;

			Search::repetitionIndex++;
			Search::repetitionTable[Search::repetitionIndex] = pos.hashKey;

			if (pos.enPassant != no_sq) // hash enpassant if available
				pos.hashKey ^= Zobrist::enPassantKeys[pos.enPassant];

			pos.enPassant = no_sq;

			pos.sideToMove ^= 1; // switching the side gives the opponent an extra move to make

			pos.hashKey ^= Zobrist::sideKey;

			score = -negamax(-beta, -beta + 1, depth - 2 - (depth >= 8 ? 3 : 2), !cutnode, pos);

			Search::ply--;
			Search::repetitionIndex--;

			takeBack(pos);

			if (pos.time.stopped == true) return 0; // returns 0 if time is up

			// fail hard beta cutoff
			if (score >= beta) return beta;
		}

		bool canFutilityPrune = false;

		if (ply && !pvNode && (depth <= 8)) {
			if ((staticEval + (168 * depth)) <= alpha) canFutilityPrune = true;
		}

		if (!pvNode && !kingCheck && depth <= 5) {
			score = staticEval + 125;

			if (score < beta) {
				int newScore;

				if (depth == 1) {
					newScore = quiescence(alpha, beta, pos);

					return (newScore > score) ? newScore : score;
				}

				score += 175;

				if (score < beta && depth <= 2) {
					newScore = quiescence(alpha, beta, pos);

					if (newScore < beta) {
						return (newScore > score) ? newScore : score;
					}
				}
			}
		}

		// internal iterative reductions
		if (cutnode && depth >= 7) depth -= 1;

		Movegen::MoveList moveList[1];

		Movegen::generateMoves(pos, moveList, false);

		if (followPV) {
			enablePVScoring(moveList);
		}

		sortMoves(moveList, bestMove, pos); // sort the moves to speed stuff up (doing the same in quiescence search)

		int movesSearched = 0;

		for (int c = 0; c < moveList->count; c++) {
			const int move = moveList->moves[c];

			copyBoard(pos);

			Search::ply++;

			Search::repetitionIndex++;
			Search::repetitionTable[Search::repetitionIndex] = pos.hashKey;

			if (pos.makeMove(pos, moveList->moves[c], allMoves) == 0) { // make sure to only make the legal moves
				Search::ply--;

				Search::repetitionIndex--;

				continue; // skip to next move
			}

			legalMoves++;

			if (movesSearched == 0) {
				score = -negamax(-beta, -alpha, depth - 1, !cutnode, pos); // doing the normal AB search
			}
			else {
				// futility pruning on current move

				if (canFutilityPrune && (legalMoves > 1)) {
					if (!pos.isSquareAttacked(Bitboards::getLs1bIndex(Bitboards::bitboards[(pos.sideToMove == Colors::white) ? Piece::K : Piece::k]), pos.sideToMove ^ 1)
						&& (killerMoves[0][ply] != move)
						&& (killerMoves[1][ply] != move)
						&& (getMovePiece(move) != Piece::P && getMovePiece(move) != Piece::p)
						&& !getMovePromotion(move)
						&& !getMoveCastling(move) && !getMoveCapture(move)) {

						repetitionIndex--;
						ply--;
						takeBack(pos);

						continue;
					}
				}

				// late move pruning
				if (ply && !pvNode && depth <= 3 && !kingCheck && !getMoveCapture(move) && (legalMoves > lmpMargins[depth])) {
					repetitionIndex--;
					ply--;
					takeBack(pos);

					continue;
				}

				// LMR
				if (movesSearched >= fullDepthMoves && depth >= reductionLimit && kingCheck == 0 && getMoveCapture(move) == 0 && getMovePromotion(move) == 0) {
					int R = 1 + depth / 4;

					if (pvNode) R--;

					score = -negamax(-alpha - 1, -alpha, depth - 1 - std::max(0, R), true, pos);
				}
				else
					score = alpha + 1;

				// principle variation search
				if (score > alpha) {
					score = -negamax(-alpha - 1, -alpha, depth - 1, false, pos); // better move has been found during LMR, re-search at full depth but with narrowed score bandwith

					// if fails to prove that other moves are bad
					if ((score > alpha) && (score < beta)) { // if LMR fails, re-search at full depth and full score bandwith
						score = -negamax(-beta, -alpha, depth - 1, !cutnode, pos);
					}
				}
			}

			Search::ply--;
			Search::repetitionIndex--;

			takeBack(pos);

			if (pos.time.stopped == true) return 0;

			movesSearched++;

			// if better move is found
			if (score > alpha) {
				// switch hash flag
				hashFlag = hashfEXACT;

				bestMove = move;

				// store history moves
				if (getMoveCapture(move) == 0)
					historyMoves[getMovePiece(move)][getMoveTarget(move)] += depth;

				alpha = score; //PV node

				pvTable[Search::ply][Search::ply] = move;

				for (int next = Search::ply + 1; next < pvLength[Search::ply + 1]; next++) {
					pvTable[Search::ply][next] = pvTable[Search::ply + 1][next];
				}

				pvLength[Search::ply] = pvLength[Search::ply + 1];

				// using fail-hard beta cutoff
				if (score >= beta) {
					// store hash entry
					writeHashEntry(beta, bestMove, depth, hashfBETA, pos);

					if (getMoveCapture(move) == 0) {
						killerMoves[1][Search::ply] = killerMoves[0][Search::ply];
						killerMoves[0][Search::ply] = move;
					}

					//report(score, depth, pos);

					return beta;
				}
			}
		}

		if (legalMoves == 0) {
			if (kingCheck) {
				return -MATE_VALUE + Search::ply;
			}
			else {
				return contemptFactor(pos);
			}
		}

		writeHashEntry(alpha, bestMove, depth, hashFlag, pos);

		//addScoreToTable(score, alpha, pos, depth, Search::ply, beta, bestMove);

		return alpha; // move fails low
	}

	static int aspirate(int depth, int score, Position& pos) {
		if (depth == 1) {
			return Search::negamax(-VALUE_INFINITE, VALUE_INFINITE, depth, false, pos);
		}

		int delta = 100;
		int alpha = std::max(score - delta, -VALUE_INFINITE);
		int beta = std::min(score + delta, VALUE_INFINITE);

		for (;; delta += delta / 2) {
			score = Search::negamax(alpha, beta, depth, false, pos);

			if (score <= alpha) {
				beta = (alpha + beta) / 2;
				alpha = std::max(alpha - delta, -VALUE_INFINITE);
			}
			else if (score >= beta) {
				alpha = (alpha + beta) / 2;
				beta = std::min(beta + delta, VALUE_INFINITE);
			}
			else {
				return score;
			}
		}
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

		//clearHashTable();

		int alpha = -VALUE_INFINITE;
		int beta = VALUE_INFINITE;

		// iterative deepening
		for (int curDepth = 1; curDepth <= depth; curDepth++) {
			if (pos.time.stopped) break; // if time is up then stop calculating and return best move so far

			followPV = 1;

			score = aspirate(curDepth, score, pos);

			if ((score <= alpha) || (score >= beta)) {
				alpha = -VALUE_INFINITE;
				beta = VALUE_INFINITE;
				continue;
			}

			alpha = score - 50;
			beta = score + 50;

			if (pvLength[0]) {
				int time = pos.time.getTimeMs() - pos.time.startTime;

				if (time == 0) time = 1;

				U64 nps = static_cast<U64>(nodes / (static_cast<double>(time) / 1000.0));

				int hashfull = hashFull();

				if (score > -MATE_VALUE && score < -MATE_SCORE) {
					printf("info score mate %d depth %d nodes %lld nps %lld hashfull %d time %d pv ", -(score + MATE_VALUE) / 2 - 1, curDepth, nodes, nps, hashfull, time);
				}
				else if (score > MATE_SCORE && score < MATE_VALUE) {
					printf("info score mate %d depth %d nodes %lld nps %lld hashfull %d time %d pv ", (MATE_VALUE - score) / 2 + 1, curDepth, nodes, nps, hashfull, time);
				}
				else
					printf("info score cp %d depth %d nodes %lld nps %lld hashfull %d time %d pv ", score, curDepth, nodes, nps, hashfull, time);


				for (int c = 0; c < pvLength[0]; c++) {
					Movegen::printMove(pvTable[0][c]);
					printf(" ");
				}

				printf("\n");
			}
		}

		printf("bestmove ");
		Movegen::printMove(pvTable[0][0]); // first element within PV table

		printf("\n");
	}
}