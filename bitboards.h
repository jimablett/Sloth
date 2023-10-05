#ifndef BITBOARDS_H
#define BITBOARDS_H

#include <cstdint>
#include <cassert>
#include <vector>

//using bitboard = uint64_t;

#define setBit(bb, sq) (bb |= (1ULL << sq))
#define getBit(bb, sq) (bb & (1ULL << sq))
#define removeBit(bb, sq) (bb &= ~(1ULL << sq))
#define popBit(bb, sq) (getBit(bb, sq) ? bb ^= (1ULL << sq) : 0) // invert bit

typedef uint64_t U64;
//typedef unsigned long long U64;

namespace Sloth {
	namespace Bitboards {
		// 12 bitboards, each piece for each color

		extern const U64 notAFile;
		extern const U64 notHFile;

		extern const U64 notHGFile;
		extern const U64 notABFile;

		extern U64 pawnAttacks[2][64]; // [side][square]
		extern U64 knightAttacks[64]; // [square]
		extern U64 kingAttacks[64];

		// MIGHT DELETE
		extern U64 bitboards[12];
		extern U64 occupancies[3]; // This will hold every piece on one bitboard. One for every white piece combined, one for black combined and one with every piece of both color on the bitboard

		extern U64 whitePawns;
		extern U64 whiteKnights;
		extern U64 whiteBishops;
		extern U64 whiteRooks;
		extern U64 whiteQueens;
		extern U64 whiteKing;

		extern U64 blackPawns;
		extern U64 blackKnights;
		extern U64 blackBishops;
		extern U64 blackRooks;
		extern U64 blackQueens;
		extern U64 blackKing;


		void printBitboard(U64 bb, bool flip);

		U64 getCombinedBitboard();
		U64 getCombinedBitboard(std::vector<U64> &bb);

		U64 maskPawnAttacks(int square, int side);
		U64 maskKnightAttacks(int square);
		U64 maskKingAttacks(int square);

		void initLeaperAttacks();

		extern inline int countBits(U64 bitboard); // counts every bit available on a bitboard

		// get least significant 1st bit index
		extern inline int getLs1bIndex(U64 bitboard);
	}
}

#endif