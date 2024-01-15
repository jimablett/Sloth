/*
CHANGES (revert if stuff go badly):

MADE getRank FUNCTION
*/

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

	struct {
		int gamePhase = -1;
		int phaseScore;
	} phase;

	struct {
		int score = 0, scoreOpening = 0, scoreEndgame = 0;
	} scores;

	struct PieceScore { // score for each piece
		int scoreOpening = 0;
		int scoreEndgame = 0;
	};

	const int doublePawnPenaltyOpening = -5;
	const int doublePawnPenaltyEndgame = -10;

	const int isolatedPawnPenaltyOpening = -5;
	const int isolatedPawnPenaltyEndgame = -10;

	const int passedPawnBonus[8] = { 0, 10, 30, 50, 75, 100, 150, 200 }; // each val corresponds to a rank (meaning +200 score if passed pawn is on 7)

	const int semiFile = 10; // score for semi open file
	const int openFile = 15; // score for open file

	U64 forwardRanksMasks[2][8];

	static const PieceScore pawnShield[] = {
		{-19, -17},
		{0, -12},
		{15, -18},
		{23, -26}
	};

	static const int bishopUnit = 4;
	static const int queenUnit = 9;

	static const int bishopMobilityOpening = 5;
	static const int bishopMobilityEnd = 5;

	static const int queenMobilityOpening = 1;
	static const int queenMobilityEnd = 2;

	static const int doubledRooks = 5;
	static const int doubledRooksEndgame = 10;

	const int kingShieldBonus = 5;

	enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

	const int materialScore[2][12] = { // game phase, piece
		82, 337, 365, 477, 1025, 12000, -82, -337, -365, -477, -1025, -12000,

		94, 281, 297, 512,  936, 12000, -94, -281, -297, -512,  -936, -12000
	};

	const int openingScore = 6192;
	const int endgameScore = 518;

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

	inline void scorePiece(PieceScore* score, int scoreOpening, int scoreEndgame) {
		score->scoreOpening += scoreOpening;
		score->scoreEndgame += scoreEndgame;
	}

	static inline U64 occupiedOnFile(int square) { // get all pieces on file
		return Bitboards::occupancies[both] & Eval::fileMasks[square];
	}

	static inline U64 occupiedOnRank(int square) {
		return Bitboards::occupancies[both] & Eval::rankMasks[square];
	}

	static inline int squareDistance(int sq1, int sq2) {
		int rankDiff = abs((sq1 / 8) - (sq2 / 8));
		int fileDiff = abs((sq2 % 8) - (sq2 % 8));

		return std::max(rankDiff, fileDiff);
	}

	static inline int getRank(int square) {
		return square / 8;
	}

	inline PieceScore evaluatePawns(int piece, int square) { // piece variable will switch between black and white pawns
		int doubled = Bitboards::countBits(Bitboards::bitboards[piece] & Eval::fileMasks[square]); // returns the amount of doubled pawns on the board for said piece side
		PieceScore score = { 0 };

		bool white = (piece == Piece::P);

		U64* passedMask = white ? Eval::wPassedMasks : Eval::bPassedMasks;

		score.scoreOpening += POSITIONAL_SCORE[opening][PAWN][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][PAWN][white ? square : MIRROR_SCORE[square]];

		if (doubled > 1) {
			scorePiece(&score, (doubled - 1) * doublePawnPenaltyOpening, (doubled - 1) * doublePawnPenaltyEndgame); // this line adds penalty to the current piece, one for opening, and one for endgame
		}

		if ((Bitboards::bitboards[piece] & Eval::isolatedMasks[square]) == 0) {
			scorePiece(&score, isolatedPawnPenaltyOpening, isolatedPawnPenaltyEndgame);
		}

		if ((passedMask[square] & Bitboards::bitboards[white ? Piece::p : Piece::P]) == 0) {
			scorePiece(&score, passedPawnBonus[getRank(square)], passedPawnBonus[getRank(square)]);
		}

		// NEW (TRY PAWN STRUCTURE)
		/**int getFile = square & 7;
		int getRank = square >> 3;

		int backwards = 0;

		U64 ours = Bitboards::bitboards[piece];

		backwards += (getFile > 0) * (getFile < 7)
			* ((ours & (Eval::isolatedMasks[getFile] >> ((8 - getRank) * 8))) != 0)
				* ((ours & (Eval::isolatedMasks[getFile] << (getRank * 8))) == 0);

		scorePiece(&score, backwards * -5, backwards * -5);*/

		return score;
	}

	inline PieceScore evaluateKnights(int piece, int square) {
		PieceScore score = { 0 };
		bool white = (piece == Piece::N);

		score.scoreOpening += POSITIONAL_SCORE[opening][KNIGHT][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][KNIGHT][white ? square : MIRROR_SCORE[square]];

		//U64 enemyPawns = Bitboards::bitboards[white ? Piece::p : Piece::P];

	//	if ((white ? enemyPawns << 8 : enemyPawns >> 8) & 1ULL << square) {
	//		scorePiece(&score, 14, 20);
	//	}

		return score;
	}

	inline PieceScore evaluateRooks(int piece, int square) {
		PieceScore score = { 0 };
		bool white = (piece == Piece::R);

		score.scoreOpening += POSITIONAL_SCORE[opening][ROOK][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][ROOK][white ? square : MIRROR_SCORE[square]];

		// semi open file
		if ((Bitboards::bitboards[white ? Piece::P : Piece::p] & Eval::fileMasks[square]) == 0) {
			scorePiece(&score, semiFile, semiFile); // adds the bonus
		}

		// open file
		if (((Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) & Eval::fileMasks[square]) == 0) {
			scorePiece(&score, openFile, openFile);
		}

		// bonus if friendly rooks are doubled
		U64 rooksOnFile = Bitboards::bitboards[piece] & Eval::fileMasks[square];
		U64 rooksOnRank = Bitboards::bitboards[piece] & Eval::rankMasks[square];

		// rooks are doubled on the file, with no pieces in between
		if (Bitboards::countBits(rooksOnFile) >= 2) {
			if ((occupiedOnFile(square) & ~rooksOnFile) == 0) {
				scorePiece(&score, doubledRooks * Bitboards::countBits(rooksOnFile), doubledRooksEndgame * Bitboards::countBits(rooksOnFile));
			}
		}

		if (Bitboards::countBits(rooksOnRank) >= 2) {
			if ((occupiedOnRank(square) & ~rooksOnRank) == 0) {
				scorePiece(&score, doubledRooks * Bitboards::countBits(rooksOnRank), doubledRooksEndgame * Bitboards::countBits(rooksOnRank));
			}
		}
		// old: 3
		//int mobility = Bitboards::countBits(Magic::getRookAttacks(square, Bitboards::bitboards[both]));
		//scorePiece(&score, mobility * 3, mobility * 3);

		// 5 is rook unit
		//int mobility = (Bitboards::countBits(Magic::getRookAttacks(square, Bitboards::occupancies[both])) - 5) * 5;

		//scorePiece(&score, mobility, mobility);

		return score;
	}

	inline PieceScore getPieceMobility(bool bishop, int square) { //  MIGHT DO: on runtime start, calculate all bishop attacks for each square so that it can be fetched from bishopAttacks array
		PieceScore score = { 0 };

		if (bishop) {
			score.scoreOpening += (Bitboards::countBits(Magic::getBishopAttacks(square, Bitboards::occupancies[both])) - bishopUnit) * bishopMobilityOpening;
			score.scoreEndgame += (Bitboards::countBits(Magic::getBishopAttacks(square, Bitboards::occupancies[both])) - bishopUnit) * bishopMobilityEnd;
		}
		else {
			score.scoreOpening += (Bitboards::countBits(Magic::getQueenAttacks(square, Bitboards::occupancies[both])) - queenUnit) * queenMobilityOpening;
			score.scoreEndgame += (Bitboards::countBits(Magic::getQueenAttacks(square, Bitboards::occupancies[both])) - queenUnit) * queenMobilityEnd;
		}

		//Bitboards::printBitboard(Magic::getBishopAttacks(square, Bitboards::occupancies[Colors::both]), false);

		return score;
	}

	inline PieceScore evaluateBishops(int piece, int square) {
		PieceScore score = { 0 };
		PieceScore mobility = getPieceMobility(true, square);
		bool white = (piece == Piece::B);

		score.scoreOpening += POSITIONAL_SCORE[opening][BISHOP][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][BISHOP][white ? square : MIRROR_SCORE[square]];

		scorePiece(&score, mobility.scoreOpening, mobility.scoreEndgame);

		return score;
	}

	inline PieceScore evaluateQueens(int piece, int square) {
		PieceScore score = { 0 };
		PieceScore mobility = getPieceMobility(false, square);
		bool white = (piece == Piece::Q);

		score.scoreOpening += POSITIONAL_SCORE[opening][QUEEN][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][QUEEN][white ? square : MIRROR_SCORE[square]];

		scorePiece(&score, mobility.scoreOpening, mobility.scoreEndgame);

		return score;
	}

	inline PieceScore evaluateKings(int piece, int square) {
		PieceScore score = { 0 };
		bool white = (piece == Piece::K);
		int kingRank = white ? getRank(square) : GET_RANK[MIRROR_SCORE[square]];

		//score += KING_SCORE[piece == Piece::K ? square : MIRROR_SCORE[square]];

		score.scoreOpening += POSITIONAL_SCORE[opening][KING][white ? square : MIRROR_SCORE[square]];
		score.scoreEndgame += POSITIONAL_SCORE[endgame][KING][white ? square : MIRROR_SCORE[square]];

		if (phase.gamePhase != endgame) { // prolly dont need this, however, intention is to make the king be more careful during the active phases of the game
			if ((Bitboards::bitboards[white ? Piece::P : Piece::p] & Eval::fileMasks[square]) == 0) { // king on semi open file
				scorePiece(&score, -semiFile, -semiFile); // -semifile will turn it into a penalty
			}

			if (((Bitboards::bitboards[Piece::P] | Bitboards::bitboards[Piece::p]) & Eval::fileMasks[square]) == 0) {
				scorePiece(&score, -openFile, -openFile);
			}

			if (kingRank == 0) {
				/*
				New king protection
				~21 elo
				*/

				U64 pawnSquares = white ? (square % 8 < 3 ? 0x007000000000000ULL : 0x000E0000000000000ULL) : (square % 8 < 3 ? 0x700 : 0xE000);
				U64 pawns = Bitboards::bitboards[white ? Piece::P : Piece::p] & pawnSquares;
				PieceScore shieldScores = pawnShield[std::min(Bitboards::countBits(pawns), 3)];

				scorePiece(&score, shieldScores.scoreOpening, shieldScores.scoreEndgame);

				/*U64 kingProtectionMask = (1ULL << square - 1) | (1ULL << square + 1);
				int safetyScore = Bitboards::countBits((Bitboards::kingAttacks[square] ^ kingProtectionMask) & Bitboards::occupancies[white ? Colors::white : Colors::black]) * kingShieldBonus;

				scorePiece(&score, safetyScore, safetyScore);*/
			}

/*			U64 enemyQueens = Bitboards::bitboards[white ? Piece::q : Piece::Q];
			
			while (enemyQueens) {
				int sq = Bitboards::getLs1bIndex(enemyQueens);
				int dist = squareDistance(square, sq);

				if (dist < 5 && dist > 1) {
					scorePiece(&score, sqrt((1 / dist)) * -100, sqrt((1 / dist)) * -100);
				}

				popBit(enemyQueens, sq);
			}*/
		}

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

		scores.score = 0;
		scores.scoreEndgame = 0;
		scores.scoreOpening = 0;

		phase.phaseScore = getGamePhaseScore();

		if (phase.phaseScore > openingScore)
			phase.gamePhase = opening;
		else if (phase.phaseScore < endgameScore)
			phase.gamePhase = endgame;
		else
			phase.gamePhase = middlegame;

		U64 bb;

		int piece, square;

		PieceScore P, N, B, R, Q, K, p, n, b, r, q, k;

		if (phase.gamePhase == endgame) {
			if (Bitboards::countBits(Bitboards::occupancies[Colors::both]) < 5) {
				// king vs king is a draw
				if ((Bitboards::occupancies[Colors::both] & ~(Bitboards::bitboards[Piece::K] | Bitboards::bitboards[Piece::k])) == 0) {
					return 0;
				}
			}
		}

		for (int bbPiece = Piece::P; bbPiece <= Piece::k; bbPiece++) {
			bb = Bitboards::bitboards[bbPiece];

			while (bb) { // loop over pieces in current bitboard
				piece = bbPiece;

				square = Bitboards::getLs1bIndex(bb);

				scores.scoreOpening += materialScore[opening][piece];
				scores.scoreEndgame += materialScore[endgame][piece];

				switch (piece)
				{
				case Piece::P:
					P = evaluatePawns(Piece::P, square);
					scores.scoreOpening += P.scoreOpening;
					scores.scoreEndgame += P.scoreEndgame;

					break;
				case Piece::N:
					N = evaluateKnights(Piece::N, square);
					scores.scoreOpening += N.scoreOpening;
					scores.scoreEndgame += N.scoreEndgame;

					break;
				case Piece::B:
					B = evaluateBishops(Piece::B, square);
					scores.scoreOpening += B.scoreOpening;
					scores.scoreEndgame += B.scoreEndgame;

					break;
				case Piece::R:
					R = evaluateRooks(Piece::R, square);
					scores.scoreOpening += R.scoreOpening;
					scores.scoreEndgame += R.scoreEndgame;

					break;
				case Piece::Q:
					Q = evaluateQueens(Piece::Q, square);
					scores.scoreOpening += Q.scoreOpening;
					scores.scoreEndgame += Q.scoreEndgame;

					break;
				case Piece::K:
					K = evaluateKings(Piece::K, square);
					scores.scoreOpening += K.scoreOpening;
					scores.scoreEndgame += K.scoreEndgame;

					break;

				case Piece::p:
					p = evaluatePawns(Piece::p, square);
					scores.scoreOpening -= p.scoreOpening;
					scores.scoreEndgame -= p.scoreEndgame;

					break;
				case Piece::n:
					n = evaluateKnights(Piece::n, square);
					scores.scoreOpening -= n.scoreOpening;
					scores.scoreEndgame -= n.scoreEndgame;

					break;
				case Piece::b:
					b = evaluateBishops(Piece::b, square);
					scores.scoreOpening -= b.scoreOpening;
					scores.scoreEndgame -= b.scoreEndgame;

					break;
				case Piece::r:
					r = evaluateRooks(Piece::r, square);
					scores.scoreOpening -= r.scoreOpening;
					scores.scoreEndgame -= r.scoreEndgame;

					break;
				case Piece::q:
					q = evaluateQueens(Piece::q, square);
					scores.scoreOpening -= q.scoreOpening;
					scores.scoreEndgame -= q.scoreEndgame;

					break;
				case Piece::k:
					k = evaluateKings(Piece::k, square);
					scores.scoreOpening -= k.scoreOpening;
					scores.scoreEndgame -= k.scoreEndgame;

					break;
				}

				popBit(bb, square);
			}
		}

		if (phase.gamePhase == middlegame) { // interpolating scores in the middlegame
			/*
				Formula used for calculating interpolated score for a given game phase:
				(openingScore * phaseScore + endgameScore * (openingPhaseScore - gamePhaseScore)) / openingPhaseScore

				EXAMPLE: score for pawn on d4 at phase 5000 would be:
				interpolatedScore = (12 * 5000 + (-7) * (6192 - 5000)) / 6192 = 8.34237726098
			*/
			scores.score = (scores.scoreOpening * phase.phaseScore + scores.scoreEndgame * (openingScore - phase.phaseScore)) / openingScore;
		}
		else if (phase.gamePhase == opening) {
			scores.score = scores.scoreOpening; // pure opening score in opening
		}
		else {
			scores.score = scores.scoreEndgame;
		}

		//int fiftyMove = (100 - pos.fifty) / 100;
		// IMPORTANT! check to see if this fifty move rule counter actually does good for eval
		//return (pos.sideToMove == Colors::white) ? scores.score * fiftyMove : -(scores.score * fiftyMove);
		
		
		return (pos.sideToMove == Colors::white) ? scores.score : -scores.score;
	}
}