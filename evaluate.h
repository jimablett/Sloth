#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include "position.h"

namespace Sloth {

	namespace Eval {
		enum PieceTypes { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NB_PIECE };

		enum Scales {
			SCALE_DRAW = 0,
			SCALE_OCB_BISHOPS_ONLY = 64,
			SCALE_OCB_ONE_KNIGHT = 106,
			SCALE_OCB_ONE_ROOK = 96,
			SCALE_LONE_QUEEN = 88,
			SCALE_NORMAL = 128,
			SCALE_LARGE_PAWN_ADV = 144,
		};

		extern U64 fileMasks[64];
		extern U64 rankMasks[64];

		extern U64 isolatedMasks[64]; // isolated pawns
		extern U64 wPassedMasks[64]; // white passed pawns
		extern U64 bPassedMasks[64]; // black
		extern U64 orgthogonalDistance[64][64];

		//extern inline int evaluateClosedness();

		U64 setFileRankMask(int fileNum, int rankNum);
		void initEvalMasks();

		extern inline int evaluate(Position& pos);
	}
}

#endif