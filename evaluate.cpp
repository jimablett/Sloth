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

	const int kingShieldBonus = 5;

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

	inline int evaluatePawns(int piece, int square) { // piece variable will switch between black and white pawns
		int doubled = Bitboards::countBits(Bitboards::bitboards[piece] & Eval::fileMasks[square]); // returns the amount of doubled pawns on the board for said piece side
		int score = 0; // score for pawns

		U64* passedMask = (piece == Piece::P) ? Eval::wPassedMasks : Eval::bPassedMasks;

		piece == Piece::P ? score += PAWN_SCORE[square] : score += PAWN_SCORE[MIRROR_SCORE[square]]; // positional score

		if (doubled > 1) {
			score += doubled * doublePawnPenalty; // adds the amount of doubled pawns multiplied by the penalty for each doubled pawn (Will result in a negative number so long as doublePawnPenalty is negative)
		}

		if ((Bitboards::bitboards[piece] & Eval::isolatedMasks[square]) == 0) { // if pawn is isolated
			score += isolatedPawnPenalty;
		}

		if ((passedMask[square] & Bitboards::bitboards[piece == Piece::P ? Piece::p : Piece:: P]) == 0) { // if pawn is passed
			score += passedPawnBonus[piece == Piece::P ? GET_RANK[square] : GET_RANK[MIRROR_SCORE[square]]];
		}

		return score;
	}

	inline int evaluateRooks(int piece, int square) {
		int score = 0;

		score += ROOK_SCORE[piece == Piece::R ? square : MIRROR_SCORE[square]];

		// semi open file
		if ((Bitboards::bitboards[piece == Piece::R ? Piece::P : Piece::p] & Eval::fileMasks[square]) == 0) {
			score += semiFile;
		}

		if (((Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) & Eval::fileMasks[square]) == 0) {
			score += openFile;
		}

		return score;
	}

	inline int evaluateKings(int piece, int square) {
		int score = 0;
		int kingRank = piece == Piece::K ? GET_RANK[square] : GET_RANK[MIRROR_SCORE[square]];

		U64 kingProtectionMask = (1ULL << square - 1) | (1ULL << square + 1); // filters out squares above and under the king

		score += KING_SCORE[piece == Piece::K ? square : MIRROR_SCORE[square]];

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

	inline int Eval::evaluate(Position& pos) {
		int score = 0;

		U64 bb;

		int piece, square;

		for (int bbPiece = Piece::P; bbPiece <= Piece::k; bbPiece++) {
			bb = Bitboards::bitboards[bbPiece];

			while (bb) { // loop over pieces in current bitboard
				piece = bbPiece;

				square = Bitboards::getLs1bIndex(bb);

				// SEE https://www.chessprogramming.org/Evaluation, MIGHT MODIFY THIS TO MAKE IT MORE ACCURATE
				// score material weights
				score += materialScore[piece];

				// TODO: create functions like evaluatepawns
				// score position piece scores
				switch (piece)
				{
				case Piece::P:
					score += evaluatePawns(Piece::P, square);

					break;
				case Piece::N: score += KNIGHT_SCORE[square]; break;
				case Piece::B:
					score += BISHOP_SCORE[square];
					
					score += getPieceMobility(true, square);

					break;
				case Piece::R:
					score += evaluateRooks(Piece::R, square);

					break;
				case Piece::Q:
					score += getPieceMobility(false, square);

					break;
				case Piece::K:
					score += evaluateKings(Piece::K, square);

					break;

				case Piece::p:
					score -= evaluatePawns(Piece::p, square);

					break;
				case Piece::n: score -= KNIGHT_SCORE[MIRROR_SCORE[square]]; break;
				case Piece::b:
					score -= BISHOP_SCORE[MIRROR_SCORE[square]];

					score -= getPieceMobility(true, square);
					
					break;
				case Piece::r:
					score -= evaluateRooks(Piece::r, square);

					break;
				case Piece::q:
					score -= getPieceMobility(false, square);

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