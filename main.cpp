/*
    Big thanks to:
    https://www.youtube.com/@chessprogramming591
    https://www.chessprogramming.org/

*/

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <string.h>

#include "uci.h"
#include "piece.h"
#include "magic.h"
#include "movegen.h"
#include "types.h"

// testing
#include "position.h"
#include "perft.h"
#include "uci.h"
#include "search.h"
#include "evaluate.h"

using namespace Sloth;

int main(int argc, char* argv[])
{
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

    //std::cout << "Sloth version 1.0.0" << std::endl;


    // default pos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    //Position pos;

    Magic::initAttacks();
    Bitboards::initLeaperAttacks();
    Zobrist::initRandomKeys();
    Search::initHashTable(64); // 64 mb default
    Eval::initEvalMasks();
    //game.parseFen(startPosition);

    int debug = 0;

    // USING OK CONFIG SEEMS TO BE WORKING? (CHANGED CONFIGS, SEE DIFFERENCES)
    if (debug) {
        Position pos;

        Movegen::MoveList movelist[1];


        // killer position without probcut: 

        pos.parseFen(startPosition);
        //pos.parseFen("8/8/8/8/3N1N2/8/2N1N3/8 w - - 0 1");

        pos.printBoard();

        Bitboards::printBitboard(Bitboards::bitboards[Piece::P], false);
        
        //Bitboards::printBitboard(~(~Bitboards::notABFile | ~Bitboards::notHGFile), false);
       // Bitboards::printBitboard(0x0505050505050505ULL, false);
        //Movegen::generateMoves(pos, movelist, false);
 
        /*Threads::Thread* thread = Threads::createThreadPool(2); // 2 threads
        UCI::UCIGoStruct uciGoStruct{thread, pos};
        UCI::Limits *limits = &uciGoStruct.limits;

        memset(limits, 0, sizeof(UCI::Limits));

        // check limits right here if not working correctly
        limits->multiPV = movelist->count;

        Search2::startSearchThreads(&uciGoStruct); // starts the search*/

        // info score cp 0 depth 12 nodes 6111926 time 6047 pv b1c3 g8f6 d2d4 d7d5 c1f4 e7e6 g1f3 f6g4
        // reverse futility pruning: info score cp 0 depth 12 nodes 5033723 time 5500 pv b1c3 g8f6 e2e3 b8c6 d2d4 d7d5 g1f3 f6g4

        // two dimensional PV table: info score cp 0 depth 12 nodes 5033723 time 4454 pv b1c3 g8f6 e2e3 b8c6 d2d4 d7d5 g1f3 f6g4
        // one dimensional PV table: info score cp 0 depth 12 nodes 3558957 time 2875 pv d2d4 g8f6 d2d4 b8c6 c1b2 b8c6 a3b4 c8e6

        //Bitboards::printBitboard(Bitboards::pawnAdvance(Bitboards::bitboards[Piece::p]));

        //Movegen::generateMoves(pos, movelist);

        //Search::sortMoves(movelist, pos);
        
        //Search::printMoveScores(movelist, pos);

        
        //Search::search(pos, 3);

        /*
        Movegen::MoveList movelist[1];
        //pos.parseFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        pos.parseFen("r1b5/kpQp3p/6r1/2p5/1P2PP2/1P6/2P4P/R1BK3R b - - 0 30");

        Movegen::generateMoves(pos, movelist);

        for (int i = 0; i < movelist->count; i++) {
            int move = movelist->moves[i];

            int score = 0;

            copyBoard(pos);

            if (pos.makeMove(pos, move, allMoves) == 0) {
                printf("illegal: ");
            }
            else {
                printf("legal: ");

                score = Search::negamax(-50000, 50000, 5, pos);
            }
            Movegen::printMove(move);

            printf(" score: %d\n", score);

            score = 0;

            takeBack(pos);

            //printf("\n%d\n", move);
        }*/

        //pos.printBoard();



        //Search::search(pos, 6);
        
    } else UCI::loop();

    free(Search::hashTable);
    //free(Search2::hashTable);


    //pos.LoadPosition(fen);
    // "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R w KQkq - 0 1"
    //pos.parseFen("R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNNK1B1 w - - 0 1");
    //pos.parseFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    //pos.printBoard();

    // UCI
    // POSITION PARSING
    //pos.parseFen(startPosition); // we somehow have to load a position before using parsePosition, will look into this later

    //UCI::parsePosition(pos, "position startpos moves e2e4 e7e5 g1f3");

    // Moves queen from f3 to e2
    //UCI::parsePosition(pos, "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R w KQkq - 0 1 moves f3e2");

    //pos.printBoard();

    //UCI::parseGo(pos, "go depth 7");


    //UCI::parsePosition(pos, "position fen ");


    // MOVE PARSING
    /* int move = UCI::parseMove(pos, "d5c6"); // BE AWARE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IT SAYS THAT CASTLING QUEEN SIDE STILL LEGAL AFTER MOVING A1 ROOK TO B1. THIS IS A BUG THAT MUST BE LOOKED AT

    if (move) {
        pos.makeMove(pos, move, allMoves);
        pos.printBoard();
    }
    else {
        printf("illegal\n");
    }*/


    /////////////////////


    // PERFT



    //Movegen::MoveList moveList[1];

    //Movegen::generateMoves(pos, moveList);

    // following perft test on default starting position gives:
    /*
    106743106+137077337+133233975+134087476+144074944+157756443+227598692+269605599+306138410+309478263+102021008+119614841+135987651+130293018+106678423+138495290+120142144+148527161+147678554+120669525

    which is 3,195,901,860, and proves that our perft test works correctly
    
    Perft::perftTest(7, pos);
    */

    //Perft::perftTest(5, pos);



    //printf("time taken: %d ms\n", getTimeMs() - start);
    //printf("Nodes: %ld\n", Perft::nodes);
    //////////////////////////


    /*for (int c = 0; c < moveList->count; c++) {
        int move = moveList->moves[c];

        // preserve board state
        copyBoard(pos);

        if (!pos.makeMove(pos, move, MoveType::allMoves)) { // skip to next move if move is illegal (Like if the king would be exposed to check).
            continue;
        }

        pos.printBoard();
        getchar();

        takeBack(pos);
        pos.printBoard();
        getchar();
    }*/

    /*
    // creates a movelist
    Movegen::MoveList moveList[1];


    // move count MUST be set to 0
    //moveList->count = 0;
    
    Movegen::generateMoves(pos, moveList);

    // adds move to movelist
    //Movegen::addMove(moveList, encodeMove(d7, e8, Piece::B, Piece::Q, 0, 0, 0, 1));

    Movegen::printMoveList(moveList);
    */

    /*int move = encodeMove(d7, e8, Piece::P, Piece::Q, 1, 0, 0, 0);

    int sourceSquare = getMoveSource(move);
    int targetSquare = getMoveTarget(move);
    int piece = getMovePiece(move);
    int promotedPiece = getMovePromotion(move);

    printf("source square: %s\n", squareToCoordinates[sourceSquare]);
    printf("target square: %s\n", squareToCoordinates[targetSquare]);
    printf("piece: %d\n", piece); // 0 is white pawn
    printf("promoted piece: %d\n", promotedPiece); // 4 is white queen
    printf("capture flag: %d\n", getMoveCapture(move) ? 1 : 0);
    printf("double pawn push flag: %d\n", getDoublePush(move) ? 1 : 0);
    printf("enpassant flag: %d\n", getMoveEnpassant(move) ? 1 : 0);
    printf("castling flag: %d\n", getMoveCastling(move) ? 1 : 0);*/

    /*int move = 0;
    
    // encode target square of h1
    move = (move | h1) << 6;

    Bitboards::printBitboard(move, false);

    // extract target square from move
    int targetSquare = (move & 0xfc0) >> 6;

    printf("target square: %d\n", targetSquare);*/

    //Bitboards::initLeaperAttacks();

    //Bitboards::printBitboard(Bitboards::pawnAttacks[Colors::white][SQ_H4], true);

    //Bitboards::printBitboard(Bitboards::occupancies[white]);
    //Bitboards::printBitboard(Bitboards::occupancies[both]);

    // TESTING GENERATE MOVES

    

    //pos.printBoard();


    //Movegen::generateMoves(pos);

    // TESTING GET ATTACKED SQUARE

    /*Bitboards::initLeaperAttacks();
    Magic::initAttacks();

    pos.printBoard();
    Bitboards::printBitboard(Bitboards::occupancies[Colors::both], false);
    pos.printAttackedSquares(Colors::white); // squares attacked by white pieces, gets the attacked squares for black side (white attacking black)
    */
    // TESTING QUEEN ATTACKS
    
    /*U64 occupancies = 0ULL;

    setBit(occupancies, SQ_B6);
    setBit(occupancies, SQ_D6);
    setBit(occupancies, SQ_F6);

    Magic::initAttacks();

    Bitboards::printBitboard(Magic::getQueenAttacks(SQ_D4, occupancies), true);*/

    //////////////////////

    // TESTING 12 NEW BITBOARDS

    //setBit(Bitboards::bitboards[Piece::P], SQ_E2); // sets a white pawn on E2

    //Bitboards::printBitboard(Bitboards::bitboards[Piece::P]); // gets the white pawns bitboard

    //////////////////////

    // TEST KNIGHT ATTACKS & KING

    //Bitboards::initLeaperAttacks(); // IMPORTANT !!!!!!!!!! MUST INIT LEAPER ATTACKS FOR PAWN/KING/KNIGHT ATTACK TABLES TO WORK

    //Bitboards::printBitboard(Bitboards::maskKingAttacks(SQ_A8));

    //Bitboards::printBitboard(Bitboards::knightAttacks[SQ_E4]);

    //Bitboards::printBitboard(Bitboards::maskKnightAttacks(SQ_E4));

    //////////////////////





    // TEST PAWN ATTACKS

    // LEFT OFF AT 23:07, SOMETHING IS WRONG IN BIT MANIPULATION AND IT SHOULD ATTACK TWO SQUARES UP INSTEAD

    //Bitboards::initLeaperAttacks(); // IMPORTANT !!!!!!!!!! MUST INIT LEAPER ATTACKS FOR PAWN/KING/KNIGHT ATTACK TABLES TO WORK

    //Bitboards::printBitboard(Bitboards::maskPawnAttacks(SQ_H4, Colors::black));

    //////////////////////


    // TEST SLIDING ATTACKS
    /*
    U64 occupancy = 0ULL;

    setBit(occupancy, SQ_G5);
    setBit(occupancy, SQ_E2);

    //Bitboards::printBitboard(occupancy);

    Magic::initAttacks(); // IMPORTANT !!!!!!!!!!! MUST INIT ATTACKS FOR MAGIC BITBOARDS TO WORK

    cout << endl;

    Bitboards::printBitboard(Magic::getBishopAttacks(SQ_D4, occupancy));
    cout << endl;
    Bitboards::printBitboard(Magic::getRookAttacks(SQ_E5, occupancy));
    */
    //////////////////////





    //Bitboards::printBitboard(Magic::maskRookAttacks(SQ_D4)); // ATTACK MASKS: just a bitboard, with the bits that pieces are attacking enabled

    //Magic::initMagicNumbers(); magic numbers are now initiated as constants, not needed anymore

    /*unsigned long long block = 0ULL;
    setBit(block, SQ_D7);
    setBit(block, SQ_D2);
    setBit(block, SQ_D1);
    setBit(block, SQ_B4);
    setBit(block, SQ_G4);

    //Bitboards::printBitboard(block & (~block + 1)); // isolates least significant bit
    //Bitboards::printBitboard((block & (~block + 1)) - 1);

    Bitboards::printBitboard(block & (0 - block));
    cout << endl;
    Bitboards::printBitboard(block);*/

    /*for (int square = 0; square < 64; square++) {
        printf(" 0x%lluxULL\n", Magic::rookMagics[square]);
    }

    for (int square = 0; square < 64; square++) {
        printf(" 0x%lluxULL\n", Magic::bishopMagics[square]);
    }*/

    //Bitboards::printBitboard(Bitboards::blackKing);

    //vector<U64> ok{Bitboards::whiteKnights, Bitboards::blackKnights};

    //Bitboards::printBitboard(Bitboards::getCombinedBitboard(ok));



    //cout << "\n" << Bitboards::getCombinedBitboard() << endl;

    //cout << Bitboards::whitePawns << endl;
    
    /*while (true) {
        std::string t;

        std::cin >> t;

        if (t == "uci") {
            std::cout << "uciok" << std::endl;
        }
        else if (t == "isready") {
            cout << "readyok" << endl;
        }
        else if (t == "go") {
            cout << "bestmove e7e5" << std::endl;
        }
    }*/
    
    //UCI::loop(argc, argv);



    return 0;
}