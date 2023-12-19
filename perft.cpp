#include <omp.h>
#include <string>
#include <iomanip>
#include <sstream>

#include "perft.h"
#include "position.h"
#include "movegen.h"

namespace Sloth {
	long Perft::nodes = 0;

	inline void Perft::perft(int depth, Position& pos) {

		if (depth == 0) {
			nodes++;

			return;
		}

		Movegen::MoveList moveList[1];

		Movegen::generateMoves(pos, moveList, false);

        for (int c = 0; c < moveList->count; c++) {

            // preserve board state
            copyBoard(pos);

            if (!pos.makeMove(pos, moveList->moves[c], MoveType::allMoves)) { // skip to next move if move is illegal (Like if the king would be exposed to check).
                continue;
            }

			perft(depth - 1, pos);

            takeBack(pos);
        }
	}

    std::string formatNumber(long long number) {
        if (number >= 1000000000) {
            double billions = static_cast<double>(number) / 1000000000.0;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << billions << 'B';
            return ss.str();
        }
        else if (number >= 1000000) {
            double millions = static_cast<double>(number) / 1000000.0;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << millions << 'M';
            return ss.str();
        }
        else if (number >= 1000) {
            double thousands = static_cast<double>(number) / 1000.0;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << thousands << 'K';
            return ss.str();
        }
        return std::to_string(number);
    }

	void Perft::perftTest(int depth, Position& pos) {
        const char* squareToCoordinates[] = {
                "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
                "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
                "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
                "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
                "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
                "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
                "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
                "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
        };

        nodes = 0;

        printf("\nPerft\n");

        Movegen::MoveList moveList[1];

        Movegen::generateMoves(pos, moveList, false);

        long start = getTimeMs();

        for (int c = 0; c < moveList->count; c++) {

            // preserve board state
            copyBoard(pos);

            if (!pos.makeMove(pos, moveList->moves[c], MoveType::allMoves)) {
                continue;
            }

            long cumulativeNodes = nodes;

            perft(depth - 1, pos);

            long oldNodes = nodes - cumulativeNodes;

            takeBack(pos);
            
            printf("Move: %s%s%c Nodes: %ld\n", squareToCoordinates[getMoveSource(moveList->moves[c])], squareToCoordinates[getMoveTarget(moveList->moves[c])], Movegen::promotedPieces[static_cast<Piece::Pieces>(getMovePromotion(moveList->moves[c]))], oldNodes);
        }

        long time = getTimeMs() - start;
        double nps = static_cast<double>(nodes) / (time / 1000.0);

        printf("\nDepth: %d\n", depth);
        printf("Nodes: %lld\n", nodes);
        printf("Nodes per second: %s\n", formatNumber(nps));
        //printf("Nodes per second: %s\n", formatNumber(nodes / ((getTimeMs() - start) / 1000)));
        printf("Time: %ld ms\n", time);
	}
}