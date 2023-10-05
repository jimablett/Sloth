#include "evaluate.h"

#include "bitboards.h"
#include "piece.h"
#include "position.h"
#include "types.h"

namespace Sloth {
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

				// score position piece scores
				switch (piece)
				{
				case Piece::P: score += PAWN_SCORE[square]; break;
				case Piece::N: score += KNIGHT_SCORE[square]; break;
				case Piece::B: score += BISHOP_SCORE[square]; break;
				case Piece::R: score += ROOK_SCORE[square]; break;
				//case Piece::Q: score += QUEEN_SCORE[square]; break;
				case Piece::K: score += KING_SCORE[square]; break;

				case Piece::p: score -= PAWN_SCORE[MIRROR_SCORE[square]]; break;
				case Piece::n: score -= KNIGHT_SCORE[MIRROR_SCORE[square]]; break;
				case Piece::b: score -= BISHOP_SCORE[MIRROR_SCORE[square]]; break;
				case Piece::r: score -= ROOK_SCORE[MIRROR_SCORE[square]]; break;
				//case Piece::q: score -= QUEEN_SCORE[MIRROR_SCORE[square]]; break;
				case Piece::k: score -= KING_SCORE[MIRROR_SCORE[square]]; break;
				}

				popBit(bb, square);
			}
		}

		return (pos.sideToMove == Colors::white) ? score : -score;
	}
}