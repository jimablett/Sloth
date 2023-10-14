#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <iostream>
#include <string>
#include <string.h>

#include "bitboards.h"
#include "time.h"

#include "types.h"


namespace Sloth {
	#define copyBoard(pos) \
		U64 bbsCopy[12], occCopies[3]; \
		int side, enPassant, castle; \
		memcpy(bbsCopy, Bitboards::bitboards, 96); \
		memcpy(occCopies, Bitboards::occupancies, 24); \
		side = pos.sideToMove, enPassant = pos.enPassant, castle = pos.castle; \
		U64 hashKeyCopy = pos.hashKey; \
	
	#define takeBack(pos) \
		memcpy(Bitboards::bitboards, bbsCopy, 96); \
		memcpy(Bitboards::occupancies, occCopies, 24); \
		pos.sideToMove = side; pos.enPassant = enPassant; pos.castle = castle; \
		pos.hashKey = hashKeyCopy; \

	class Position {
	public:
		int sideToMove = -1;
		int enPassant = no_sq; // en passant square
		int castle;

		U64 hashKey = 0ULL;

		int makeMove(Position& pos, int move, int moveFlag); // IF TOO SLOW THEN TRY USING STATIC INLINE, HAVE IT DEFINED IN HEADER FILE

		Position parseFen(const char *fen);

		void printBoard();

		inline int isSquareAttacked(int square, int side);

		// temp
		void printAttackedSquares(int side);

		Time time; // time holder for the position
	};

	extern Position game; // this game is going to be the class for every chess game that the engine plays

	namespace Zobrist {
		extern U64 pieceKeys[12][4];
		extern U64 enPassantKeys[64];
		extern U64 castlingKeys[16];
		extern U64 sideKey;

		void initRandomKeys();
		U64 generateHashKey(Position& pos);
	}
}

#endif