#include "evaluate.h"

#include "bitboards.h"
#include "piece.h"
#include "position.h"
#include "magic.h"
#include "types.h"

namespace Sloth {
	U64 Eval::fileMasks[64];
	U64 Eval::rankMasks[64];

	U64 Eval::isolatedMasks[64];
	U64 Eval::wPassedMasks[64];
	U64 Eval::bPassedMasks[64];

	const int doublePawnPenalty = -10;
	const int isolatedPawnPenalty = -10;
	const int passedPawnBonus[8] = {0, 5, 10, 20, 35, 60, 100, 200}; // each val corresponds to a rank (meaning +200 score if passed pawn is on 7)

	const int semiFile = 10; // score for semi open file
	const int openFile = 15; // score for open file

	enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

	const int materialScore[2][12] = { // game phase, piece
		82, 337, 365, 477, 1025, 12000, -82, -337, -365, -477, -1025, -12000,

		94, 281, 297, 512,  936, 12000, -94, -281, -297, -512,  -936, -12000
	};

	const int openingScore = 6192;
	const int endgameScore = 518;

	const int kingShieldBonus = 5;

	struct {
		int gamePhase;
		int phaseScore;
	} phase;

	U64 Eval::setFileRankMask(int fileNum, int rankNum) {
		U64 mask = 0ULL;

		for (int i = 0; i < 8; i++) {
			if (fileNum != -1) {
				mask |= setBit(mask, i * 8 + fileNum);
			}
			else if (rankNum != -1) {
				mask |= setBit(mask, rankNum * 8 + i);
			}
		}

		return mask;
	}

	void Eval::initEvalMasks() {

		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				int sq = rank * 8 + file;

				fileMasks[sq] |= setFileRankMask(file, -1);
				rankMasks[sq] |= setFileRankMask(-1, rank);

				isolatedMasks[sq] |= setFileRankMask(file - 1, -1);
				isolatedMasks[sq] |= setFileRankMask(file + 1, -1);

				//Bitboards::printBitboard(isolatedMasks[sq], false); // if isolated masks bugged then look at this
			}
		}

		for (int rank = 0; rank < 8; rank++) { // ugly but it works
			for (int file = 0; file < 8; file++) {
				int sq = rank * 8 + file;

				wPassedMasks[sq] |= setFileRankMask(file - 1, -1);
				wPassedMasks[sq] |= setFileRankMask(file, -1);
				wPassedMasks[sq] |= setFileRankMask(file + 1, -1);

				bPassedMasks[sq] = wPassedMasks[sq]; // doing this might be incorrect?

				for (int i = 0; i < (8 - rank); i++) {
					// reset the redundant bits
					wPassedMasks[sq] &= ~rankMasks[(7 - i) * 8 + file];
					//bPassedMasks[sq] &= ~rankMasks[i * 8 + file];
				}

				for (int i = 0; i < rank + 1; i++) {
					bPassedMasks[sq] &= ~rankMasks[i * 8 + file];
				}
			}
		}
	}

	const int GET_RANK[64] =
	{
		7, 7, 7, 7, 7, 7, 7, 7,
		6, 6, 6, 6, 6, 6, 6, 6,
		5, 5, 5, 5, 5, 5, 5, 5,
		4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2,
		1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	static inline int interpolate(int pieceType, int square, bool mirror) {
		return (POSITIONAL_SCORE[opening][pieceType][mirror ? MIRROR_SCORE[square] : square] * phase.phaseScore + (POSITIONAL_SCORE[endgame][pieceType][mirror ? MIRROR_SCORE[square] : square]) * (openingScore - phase.phaseScore)) / openingScore;
	}

	inline int evaluatePawns(int piece, int square) { // piece variable will switch between black and white pawns
		int doubled = Bitboards::countBits(Bitboards::bitboards[piece] & Eval::fileMasks[square]); // returns the amount of doubled pawns on the board for said piece side
		int score = 0; // score for pawns

		U64* passedMask = (piece == Piece::P) ? Eval::wPassedMasks : Eval::bPassedMasks;

		if (phase.gamePhase == middlegame) {
			score += interpolate(PAWN, square, (piece == Piece::P ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][PAWN][(piece == Piece::P) ? square : MIRROR_SCORE[square]];
		}

		/*if (doubled > 1) {
			score += doubled * doublePawnPenalty; // adds the amount of doubled pawns multiplied by the penalty for each doubled pawn (Will result in a negative number so long as doublePawnPenalty is negative)
		}

		if ((Bitboards::bitboards[piece] & Eval::isolatedMasks[square]) == 0) { // if pawn is isolated
			score += isolatedPawnPenalty;
		}

		if ((passedMask[square] & Bitboards::bitboards[piece == Piece::P ? Piece::p : Piece:: P]) == 0) { // if pawn is passed
			score += passedPawnBonus[piece == Piece::P ? GET_RANK[square] : GET_RANK[MIRROR_SCORE[square]]];
		}*/

		return score;
	}

	inline int evaluateKnights(int piece, int square) {
		int score = 0;

		if (phase.gamePhase == middlegame) {
			score += interpolate(KNIGHT, square, (piece == Piece::N ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][KNIGHT][(piece == Piece::N) ? square : MIRROR_SCORE[square]];
		}

		return score;
	}

	inline int evaluateRooks(int piece, int square) {
		int score = 0;

		if (phase.gamePhase == middlegame) {
			score += interpolate(ROOK, square, (piece == Piece::R ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][ROOK][(piece == Piece::R) ? square : MIRROR_SCORE[square]];
		}

		// semi open file
		if ((Bitboards::bitboards[piece == Piece::R ? Piece::P : Piece::p] & Eval::fileMasks[square]) == 0) {
			score += semiFile;
		}

		if (((Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) & Eval::fileMasks[square]) == 0) {
			score += openFile;
		}

		return score;
	}

	inline int evaluateBishops(int piece, int square) {
		int score = 0;

		if (phase.gamePhase == middlegame) {
			score += interpolate(BISHOP, square, (piece == Piece::B ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][BISHOP][(piece == Piece::B) ? square : MIRROR_SCORE[square]];
		}

		return score;
	}

	inline int evaluateQueens(int piece, int square) {
		int score = 0;

		if (phase.gamePhase == middlegame) {
			score += interpolate(QUEEN, square, (piece == Piece::Q ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][QUEEN][(piece == Piece::Q) ? square : MIRROR_SCORE[square]];
		}

		return score;
	}

	inline int evaluateKings(int piece, int square) {
		int score = 0;
		int kingRank = piece == Piece::K ? GET_RANK[square] : GET_RANK[MIRROR_SCORE[square]];


		if (phase.gamePhase == middlegame) {
			score += interpolate(KING, square, (piece == Piece::K ? false : true));
		}
		else {
			score += POSITIONAL_SCORE[phase.gamePhase][KING][(piece == Piece::K) ? square : MIRROR_SCORE[square]];
		}

		//score += KING_SCORE[piece == Piece::K ? square : MIRROR_SCORE[square]];

		if (phase.gamePhase != endgame) {
			U64 kingProtectionMask = (1ULL << square - 1) | (1ULL << square + 1); // filters out squares above and under the king

			if ((Bitboards::bitboards[piece == Piece::K ? Piece::P : Piece::p] & Eval::fileMasks[square]) == 0) {
				score -= semiFile; // now a penalty
			}

			if (((Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) & Eval::fileMasks[square]) == 0) {
				score -= openFile;
			}

			if (kingRank == 0) { // might remove this (if so then remove protection masking too)
				// CONSIDER EVENTUALLY: we probably dont need that much king protection in the endgame, where there are no queens. Therefore, go lighter on king safety in endgames
				// bitwise XOR to extract squares that are above and under king in king attacks
				score += Bitboards::countBits((Bitboards::kingAttacks[square] ^ kingProtectionMask) & Bitboards::occupancies[piece == Piece::K ? Colors::white : Colors::black]) * kingShieldBonus;
			}
		}

		return score;
	}

	inline int getPieceMobility(bool bishop, int square) { //  MIGHT DO: on runtime start, calculate all bishop attacks for each square so that it can be fetched from bishopAttacks array
		int score = 0;

		bishop ? 
			score += Bitboards::countBits(Magic::getBishopAttacks(square, Bitboards::occupancies[Colors::both]))
			: score += Bitboards::countBits(Magic::getQueenAttacks(square, Bitboards::occupancies[Colors::both]));

		//Bitboards::printBitboard(Magic::getBishopAttacks(square, Bitboards::occupancies[Colors::both]), false);

		return score;
	}

	static inline int getGamePhaseScore() {
		// piece scores
		int w = 0;
		int b = 0;

		// might be a quicker and more efficient way to do this part
		for (int p = Piece::N; p <= Piece::Q; p++)
			w += Bitboards::countBits(Bitboards::bitboards[p]) * materialScore[opening][p];
		
		for (int p = Piece::n; p <= Piece::q; p++)
			b += Bitboards::countBits(Bitboards::bitboards[p]) * -materialScore[opening][p];

		return w + b;
	}

	inline int Eval::evaluate(Position& pos) {

		phase.phaseScore = getGamePhaseScore();

		if (phase.phaseScore > openingScore)
			phase.gamePhase = opening;
		else if (phase.gamePhase < endgameScore)
			phase.gamePhase = endgame;
		else
			phase.gamePhase = middlegame;

		int score = 0;

		U64 bb;

		int piece, square;

		for (int bbPiece = Piece::P; bbPiece <= Piece::k; bbPiece++) {
			bb = Bitboards::bitboards[bbPiece];

			while (bb) { // loop over pieces in current bitboard
				piece = bbPiece;

				square = Bitboards::getLs1bIndex(bb);

				// score material weights with pure phase scores
				
				if (phase.gamePhase == middlegame) {
					/*
						Formula used for calculating interpolated score for a given game phase:
						(openingScore * phaseScore + endgameScore * (openingPhaseScore - gamePhaseScore)) / openingPhaseScore

						EXAMPLE: score for pawn on d4 at phase 5000 would be:
						interpolatedScore = (12 * 5000 + (-7) * (6192 - 5000)) / 6192 = 8.34237726098
					*/
					score += (materialScore[opening][piece] * phase.phaseScore + materialScore[endgame][piece] * (openingScore - phase.phaseScore)) / openingScore;
				}
				else {
					score += materialScore[phase.gamePhase][piece];
				}

				// SEE https://www.chessprogramming.org/Evaluation, MIGHT MODIFY THIS TO MAKE IT MORE ACCURATE
				// score material weights
				//score += materialScore[piece];
				// 
				// TODO: create functions like evaluatepawns
				// score position piece scores
				switch (piece)
				{
				case Piece::P:
					score += evaluatePawns(Piece::P, square);

					break;
				case Piece::N:
					score += evaluateKnights(Piece::N, square);

					break;
				case Piece::B:
					score += evaluateBishops(Piece::B, square);
					
					score += getPieceMobility(true, square);

					break;
				case Piece::R:
					score += evaluateRooks(Piece::R, square);

					break;
				case Piece::Q:
					score += evaluateQueens(Piece::Q, square);
					//score += getPieceMobility(false, square);

					break;
				case Piece::K:
					score += evaluateKings(Piece::K, square);

					break;

				case Piece::p:
					score -= evaluatePawns(Piece::p, square);

					break;
				case Piece::n:
					score -= evaluateKnights(Piece::n, square);

					break;
				case Piece::b:
					score -= evaluateBishops(Piece::b, square);
					score -= getPieceMobility(true, square);
					
					break;
				case Piece::r:
					score -= evaluateRooks(Piece::r, square);

					break;
				case Piece::q:
					score -= evaluateQueens(Piece::q, square);
					//score -= getPieceMobility(false, square);

					break;
				case Piece::k:
					score -= evaluateKings(Piece::k, square);

					break;
				}

				popBit(bb, square);
			}
		}

		return (pos.sideToMove == Colors::white) ? score : -score;
	}
}