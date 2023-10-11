#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include "position.h"

namespace Sloth {
	namespace Eval {
		extern U64 fileMasks[64];
		extern U64 rankMasks[64];

		extern U64 isolatedMasks[64]; // isolated pawns
		extern U64 wPassedMasks[64]; // white passed pawns
		extern U64 bPassedMasks[64]; // black

		U64 setFileRankMask(int fileNum, int rankNum);
		void initEvalMasks();

		extern inline int evaluate(Position& pos);
	}
}

#endif