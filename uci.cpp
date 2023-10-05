#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <stdlib.h>

#include "uci.h"

#include "movegen.h"
#include "piece.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "perft.h"

namespace Sloth {
	int UCI::parseMove(Position& pos, const char* moveString) {
		const char* squareToCoordinates[] = { // temporary
				"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
				"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
				"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
				"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
				"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
				"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
				"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
				"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
		};

		// MIGHT BE A FASTER AND BETTER WAY OF DOING THIS (like without generating moves all over again)
		Movegen::MoveList moveList[1];

		Movegen::generateMoves(pos, moveList);

		int sourceSquare = (moveString[0] - 'a') + (8 - (moveString[1] - '0')) * 8;
		int targetSquare = (moveString[2] - 'a') + (8 - (moveString[3] - '0')) * 8;

		//printf("source square: %s Target square: %s\n", squareToCoordinates[sourceSquare], squareToCoordinates[targetSquare]);

		// this part especially might be slow
		for (int c = 0; c < moveList->count; c++) {
			int move = moveList->moves[c];

			if (sourceSquare == getMoveSource(move) && targetSquare == getMoveTarget(move)) {
				int promoted = getMovePromotion(move);

				if (promoted) {
					char pIndex = moveString[4];

					// remake this in the future
					if ((promoted == Piece::Q || promoted == Piece::q) && pIndex == 'q') {
						return move;
					}
					else if ((promoted == Piece::R || promoted == Piece::r) && pIndex == 'r') {
						return move;
					}
					else if ((promoted == Piece::B || promoted == Piece::b) && pIndex == 'b') {
						return move;
					}
					else if ((promoted == Piece::N || promoted == Piece::n) && pIndex == 'n') {
						return move;
					}

					continue;
				}

				return move; // legal move
			}
		}

		return 0;
	}

	void UCI::parsePosition(Position& pos, const char* command) {
		command += 9; // shift pointer past "position" (Like in "position startpos")
		
		// allocate memory for copy of const char* command
		char* cmdCpy = new char[strlen(command) + 1];
		strcpy_s(cmdCpy, sizeof(char) * (strlen(command) + 1), command);
		
		//char* curChar = strstr(cmdCpy, "fen");
		char* curChar = cmdCpy;

		if (strncmp(command, "startpos", 8) == 0) { // startpos found
			// init chess board with start pos
			pos.parseFen(startPosition);

		}
		else {
			// parse fen command

			curChar = strstr(cmdCpy, "fen");

			if (curChar == NULL) {
				pos.parseFen(startPosition);
			}
			else {
				// substring found
				curChar += 4;

				pos.parseFen(curChar); // loads the fen string in "position fen xxxxxxxxxxxxxxxxx"
			}
		}

		curChar = strstr(cmdCpy, "moves");

		if (curChar != NULL) {
			curChar += 6;

			while (*curChar) { // loops over moves withing move string
				int move = parseMove(pos, curChar);

				if (move == 0) break;

				pos.makeMove(pos, move, MoveType::allMoves);

				while (*curChar && *curChar != ' ') curChar++; // moves cur char pointer to end of cur move

				// go to next move
				curChar++;
			}

		}
			pos.printBoard();

		// free the allocated memory
		delete[] cmdCpy;
	}

	void UCI::parseGo(Position& pos, const char* command) {
		int depth = -1;

		char* cmdCpy = new char[strlen(command) + 1];
		strcpy_s(cmdCpy, sizeof(char) * (strlen(command) + 1), command);

		char* curDepth = NULL;

		if (curDepth = strstr(cmdCpy, "depth")) {
			// convert str to int and assign result val to depth
			depth = atoi(curDepth + 6); // points to the number in go depth 7
		}
		else {
			depth = 4; // diff time control placeholder
		}

		Search::search(pos, depth);

		//printf("Depth: %d\n", depth);

		delete[] cmdCpy;
	}

	void UCI::loop() {
		// reset STDIN and STDOUT buffers
		setvbuf(stdin, NULL, _IONBF, 0);
		setvbuf(stdout, NULL, _IONBF, 0);

		char input[2000];

		printf("id name Sloth 1.0.0\n");
		printf("id author William Sjolund\n");
		printf("uciok\n");

		while (true) {
			memset(input, 0, sizeof(input));
			fflush(stdout);

			if (!fgets(input, 2000, stdin)) continue;

			if (input[0] == '\n') continue;

			if (strncmp(input, "isready", 7) == 0) {
				printf("readyok\n");

				continue;
			} 
			else if (strncmp(input, "position", 8) == 0) {
				parsePosition(game, input);
			}
			else if (strncmp(input, "ucinewgame", 10) == 0) {
				game.parseFen(startPosition); // THIS IS TEMPORARY UNTIL PROBLEM IS FIXED

				parsePosition(game, "position startpos");
			}
			else if (strncmp(input, "go", 2) == 0) {
				parseGo(game, input);
			}
			else if (strncmp(input, "quit", 4) == 0) {
				break;
			}
			else if (strncmp(input, "uci", 3) == 0) {
				printf("id name Sloth 1.0.0\n");
				printf("id author William Sjolund\n");
				printf("uciok\n");
			}
		}
	}

}