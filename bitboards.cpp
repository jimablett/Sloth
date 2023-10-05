#include <iostream>
#include <vector>
#include <cstdint>

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

	U64 Bitboards::whitePawns = 0;
	U64 Bitboards::whiteKnights = 0;
	U64 Bitboards::whiteBishops = 0;
	U64 Bitboards::whiteRooks = 0;
	U64 Bitboards::whiteQueens = 0;
	U64 Bitboards::whiteKing = 0;

	U64 Bitboards::blackPawns = 0;
	U64 Bitboards::blackKnights = 0;
	U64 Bitboards::blackBishops = 0;
	U64 Bitboards::blackRooks = 0;
	U64 Bitboards::blackQueens = 0;
	U64 Bitboards::blackKing = 0;


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

	U64 Bitboards::getCombinedBitboard() {
		return (Bitboards::whitePawns | Bitboards::whiteKnights | Bitboards::whiteBishops | Bitboards::whiteRooks | Bitboards::whiteRooks | Bitboards::whiteQueens | Bitboards::whiteKing
			| Bitboards::blackPawns | Bitboards::blackKnights | Bitboards::blackBishops | Bitboards::blackRooks | Bitboards::blackQueens | Bitboards::blackKing);
	}

	U64 Bitboards::getCombinedBitboard(std::vector<U64> &bb) {
		U64 mergedBitboard = 0;

		for (const U64 &bitboard : bb) {
			mergedBitboard |= bitboard;
		}

		return mergedBitboard;
	}

	U64 Bitboards::maskPawnAttacks(int square, int side) {
		U64 attacks = 0ULL;
		U64 bitboard = 0ULL; // piece bb

		// sets the piece on board
		setBit(bitboard, square);

		if (!side) { // white pawns
			/*if ((bitboard >> 7) & notAFile) attacks |= (bitboard << 7); // Should be right shift but somehow doing the opposite, therefore using left shift
			if ((bitboard >> 9) & notHFile) attacks |= (bitboard << 9);*/

			if ((bitboard >> 7) & notAFile) attacks |= (bitboard >> 7);
			if ((bitboard >> 9) & notHFile) attacks |= (bitboard >> 9);

			//if ((bitboard << 7) & notHFile) attacks |= (bitboard << 7);
			//if ((bitboard << 9) & notAFile) attacks |= (bitboard << 9);
		}
		else {
			//if ((bitboard >> 7) & notAFile) attacks |= (bitboard >> 7); // Should be left shift but somehow doing the opposite, therefore using right shift
			//if ((bitboard >> 9) & notHFile) attacks |= (bitboard >> 9);

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
		int c = 0;

		while (bb) {
			c++;

			// reset least significant 1st bit
			bb &= bb - 1;
		}

		return c;
	}

	inline int Bitboards::getLs1bIndex(U64 bitboard) {
		if (bitboard) {
			// -v can be faked with ~v + 1

			return countBits((bitboard & (0 - bitboard)) - 1);

			//return countBits((U64)(bitboard & (~bitboard + 1)) - 1);
			//return countBits((bitboard & -bitboard) - 1);

			// works?
			//U64 negatedBitboard = ~bitboard + 1;

			//return countBits(negatedBitboard - 1);


			//return countBits(bitboard - 1);


			//uint64_t signedBitboard = static_cast<uint64_t>(bitboard);
			//return countBits((signedBitboard & -signedBitboard) - 1);
			//return countBits((bitboard & -bitboard) - 1);
		}
		else {
			return -1;
		}
	}

		
}