#include <iostream>
#include <vector>
#include <cstdint>
#include <immintrin.h>
#include <intrin.h>

#include "magic.h"
#include "piece.h"
#include "bitboards.h"
#include "types.h"

namespace Sloth {
	const U64 Bitboards::notAFile = 18374403900871474942ULL;
	const U64 Bitboards::notHFile = 9187201950435737471ULL;

	const U64 Bitboards::notHGFile = 4557430888798830399ULL;
	const U64 Bitboards::notABFile = 18229723555195321596ULL;

	U64 Bitboards::pawnAttacks[2][64];
	U64 Bitboards::knightAttacks[64];
	U64 Bitboards::kingAttacks[64];

	U64 Bitboards::bitboards[12];
	U64 Bitboards::occupancies[3];

	void Bitboards::printBitboard(U64 bb, bool flip) {

		// OLD
		if (flip) {
			for (int rank = 7; rank >= 0; rank--) {
				for (int file = 0; file < 8; file++) {
					int square = rank * 8 + file;
					char piece = (bb & (1ULL << square)) ? '1' : '0';

					std::cout << piece << ' ';
				}

				std::cout << std::endl;
			}
		}
		else {
			for (int rank = 0; rank < 8; rank++) {
				for (int file = 0; file < 8; file++) {
					int square = rank * 8 + file;
					char piece = (bb & (1ULL << square)) ? '1' : '0';

					std::cout << piece << ' ';
				}

				std::cout << std::endl;
			}
		}


		printf("\n\nBitboard: %llud\n\n", bb);
	}

	U64 Bitboards::openFileCount(U64 pawns) {
		pawns |= pawns >> 8; pawns |= pawns >> 16; pawns |= pawns >> 32;

		return countBits(~pawns & 0xFF);
	}

	U64 Bitboards::pawnAdvance(U64 pawns, U64 occ, int color) {
		return ~occ & (color == Colors::white ? (pawns >> 8) : (pawns << 8)); // check these pawn pushes
	}

	U64 Bitboards::maskPawnAttacks(int square, int side) {
		U64 attacks = 0ULL;
		U64 bitboard = 0ULL; // piece bb

		// sets the piece on board
		setBit(bitboard, square);

		if (!side) { // white pawns
			if ((bitboard >> 7) & notAFile) attacks |= (bitboard >> 7);
			if ((bitboard >> 9) & notHFile) attacks |= (bitboard >> 9);
		}
		else {
			if ((bitboard << 7) & notHFile) attacks |= (bitboard << 7);
			if ((bitboard << 9) & notAFile) attacks |= (bitboard << 9);
		}

		return attacks; // attack map returned
	}

	U64 Bitboards::maskKnightAttacks(int square) {
		U64 attacks = 0ULL;
		U64 bitboard = 0ULL;

		setBit(bitboard, square);

		if ((bitboard >> 17) & notHFile) attacks |= (bitboard >> 17);
		if ((bitboard >> 15) & notAFile) attacks |= (bitboard >> 15);
		if ((bitboard >> 10) & notHGFile) attacks |= (bitboard >> 10);
		if ((bitboard >> 6) & notABFile) attacks |= (bitboard >> 6);
	
		if ((bitboard << 17) & notAFile) attacks |= (bitboard << 17);
		if ((bitboard << 15) & notHFile) attacks |= (bitboard << 15);
		if ((bitboard << 10) & notABFile) attacks |= (bitboard << 10);
		if ((bitboard << 6) & notHGFile) attacks |= (bitboard << 6);

		return attacks;
	}

	U64 Bitboards::maskKingAttacks(int square) {
		U64 attacks = 0ULL;
		U64 bitboard = 0ULL;

		setBit(bitboard, square);

		if (bitboard >> 8) attacks |= (bitboard >> 8);
		if ((bitboard >> 9) & notHFile) attacks |= (bitboard >> 9);
		if ((bitboard >> 7) & notAFile) attacks |= (bitboard >> 7);
		if ((bitboard >> 1) & notHFile) attacks |= (bitboard >> 1);

		if (bitboard << 8) attacks |= (bitboard << 8);
		if ((bitboard << 9) & notAFile) attacks |= (bitboard << 9);
		if ((bitboard << 7) & notHFile) attacks |= (bitboard << 7);
		if ((bitboard << 1) & notAFile) attacks |= (bitboard << 1);
		
		return attacks;
	}

	void Bitboards::initLeaperAttacks() {
		for (int square = 0; square < 64; square++) {
			Bitboards::pawnAttacks[Colors::white][square] = maskPawnAttacks(square, Colors::white);
			Bitboards::pawnAttacks[Colors::black][square] = maskPawnAttacks(square, Colors::black);

			Bitboards::knightAttacks[square] = maskKnightAttacks(square);

			Bitboards::kingAttacks[square] = maskKingAttacks(square);
		}

	}

	inline int Bitboards::countBits(U64 bb) {
		return _mm_popcnt_u64(bb);
		/*int c = 0;

		while (bb) {
			c++;

			// reset least significant 1st bit
			bb &= bb - 1;
		}

		return c;*/
	}

	inline int Bitboards::getLs1bIndex(U64 bitboard) {
		if (bitboard) {
			unsigned long index;

			_BitScanForward64(&index, bitboard);

			return index;
		//return countBits((bitboard & (0 - bitboard)) - 1);
		}
		else {
			return -1;
		}
	}	

	inline int Bitboards::msb(U64 bitboard) {
		unsigned long index;

		_BitScanReverse64(&index, bitboard);

		return index;
	}
}