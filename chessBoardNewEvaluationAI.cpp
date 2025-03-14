#include <iostream>
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <numeric>
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>
#include <bits/stdc++.h>
#include <ncurses.h>
#include <cmath>
#include <tuple>
#include <bitset>
using namespace std;
using Bitboard = uint64_t;

//predefinedFunctions
char getPieceCharacter(int pieceNum);
void getPlayerInput();
void scrnClear();
int beginAiMove();

bool gameActive = true;

bool aiEnabled = true;
bool debugMiniMax = false;

class Chessboard{
    public:
        int currentMove = 0;
        int curSel[2] = {0,0};
        //pieces
        Bitboard bPawns   = 0x000000000000FF00;
        Bitboard bRooks   = 0x0000000000000081;
        Bitboard bKnights = 0x0000000000000042;
        Bitboard bBishops = 0x0000000000000024;
        Bitboard bQueen   = 0x0000000000000010;
        Bitboard bKing    = 0x0000000000000008;
    
        Bitboard wPawns   = 0x00FF000000000000;
        Bitboard wRooks   = 0x8100000000000000;
        Bitboard wKnights = 0x4200000000000000;
        Bitboard wBishops = 0x2400000000000000;
        Bitboard wQueen   = 0x1000000000000000;
        Bitboard wKing    = 0x0800000000000000;
        
        //general locations
        Bitboard occupiedByBlack = 0;
        Bitboard occupiedByWhite = 0;
        
        //previous boardstate evals 
        int prevWhiteEval = 0;
        int prevBlackEval = 0;
        
        //rights
        int castlingRights[4] = {1,1,1,1};
        Bitboard bPawnPromotionRow = 0xFF00000000000000;
        Bitboard wPawnPromotionRow = 0x00000000000000FF;
        
        //precomputed valid move storage
        Bitboard moveSets[64];
        Bitboard moveSetsUnfiltered[64];
        Bitboard moveSetsCapt[64];
        Bitboard wPawnStartingRow = 0x000000000000FF00;
        Bitboard bPawnStartingRow = 0x00FF000000000000;
        
        //tile control weighting
        Bitboard centerSpaces = 0x0000001800180000;
        Bitboard outCenterSpaces = 0x0000002424240000;
        Bitboard whitePawnAdvanceOne = 0x0000000000FF0000;
        Bitboard whitePawnAdvanceTwo = 0x00000000FF000000;
        Bitboard blackPawnAdvanceOne = 0x0000FF0000000000;
        Bitboard blackPawnAdvanceTwo = 0x000000FF00000000;
        
        //move weighting tables
        int pawnTable[8][8] = {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            {50, 50, 50, 50, 50, 50, 50, 50},
            {10, 10, 20, 30, 30, 20, 10, 10},
            { 5,  5, 10, 25, 25, 10,  5,  5},
            { 0,  0,  0, 20, 20,  0,  0,  0},
            { 5, -5,-10,  0,  0,-10, -5,  5},
            { 5, 10, 10,-20,-20, 10, 10,  5},
            { 0,  0,  0,  0,  0,  0,  0,  0}
        };
        
        int knightTable[8][8] = {
            {-50,-40,-30,-30,-30,-30,-40,-50},
            {-40,-20,  0,  5,  5,  0,-20,-40},
            {-30,  5, 10, 15, 15, 10,  5,-30},
            {-30,  0, 15, 20, 20, 15,  0,-30},
            {-30,  5, 15, 20, 20, 15,  5,-30},
            {-30,  0, 10, 15, 15, 10,  0,-30},
            {-40,-20,  0,  0,  0,  0,-20,-40},
            {-50,-40,-30,-30,-30,-30,-40,-50}
        };
        
        int bishopTable[8][8] = {
            {-20,-10,-10,-10,-10,-10,-10,-20},
            {-10,  5,  0,  0,  0,  0,  5,-10},
            {-10, 10, 10, 10, 10, 10, 10,-10},
            {-10,  0, 10, 10, 10, 10,  0,-10},
            {-10,  5,  5, 10, 10,  5,  5,-10},
            {-10,  0,  5, 10, 10,  5,  0,-10},
            {-10,  0,  0,  0,  0,  0,  0,-10},
            {-20,-10,-10,-10,-10,-10,-10,-20}
        };

        int rookTable[8][8] = {
            { 0,  0,  0,  5,  5,  0,  0,  0},
            {-5,  0,  0,  0,  0,  0,  0, -5},
            {-5,  0,  0,  0,  0,  0,  0, -5},
            {-5,  0,  0,  0,  0,  0,  0, -5},
            {-5,  0,  0,  0,  0,  0,  0, -5},
            {-5,  0,  0,  0,  0,  0,  0, -5},
            { 5, 10, 10, 10, 10, 10, 10,  5},
            { 0,  0,  0,  5,  5,  0,  0,  0}
        };
        
        int queenTable[8][8] = {
            {-20,-10,-10, -5, -5,-10,-10,-20},
            {-10,  0,  5,  0,  0,  0,  0,-10},
            {-10,  5,  5,  5,  5,  5,  0,-10},
            {  0,  0,  5,  5,  5,  5,  0, -5},
            { -5,  0,  5,  5,  5,  5,  0, -5},
            {-10,  0,  5,  5,  5,  5,  0,-10},
            {-10,  0,  0,  0,  0,  0,  0,-10},
            {-20,-10,-10, -5, -5,-10,-10,-20}
        };

        int kingTable[8][8] = {
            {-30,-40,-40,-50,-50,-40,-40,-30},
            {-30,-40,-40,-50,-50,-40,-40,-30},
            {-30,-40,-40,-50,-50,-40,-40,-30},
            {-30,-40,-40,-50,-50,-40,-40,-30},
            {-20,-30,-30,-40,-40,-30,-30,-20},
            {-10,-20,-20,-20,-20,-20,-20,-10},
            { 20, 20,  0,  0,  0,  0, 20, 20},
            { 20, 30, 10,  0,  0, 10, 30, 20}
        };

        //board information
        array<int, 17> pieceMaterials = {0, 100,99999,900,300,330,500,0,0,0,0,100,99999,900,300,330,500};
        
    int getTileOccupation(int row, int col){
        Bitboard tile = 1ULL << (row*8+col);
        Bitboard* pieceBitboards[]{
            nullptr, &wPawns, &wKing, &wQueen, &wBishops, &wKnights, &wRooks,
            nullptr, nullptr, nullptr, nullptr,
            &bPawns, &bKing, &bQueen, &bBishops, &bKnights, &bRooks
        };
        for(int curPiece = 0; curPiece < 17; curPiece++){
            if(pieceBitboards[curPiece] != nullptr){
                if(*(pieceBitboards[curPiece]) & tile){
                    return curPiece;
                }
            }
        }
        return 0;
    }
    
    void printBoard(Bitboard selectedSquares){
        scrnClear();
        occupiedByWhite = wPawns | wKing | wQueen | wKnights | wBishops | wRooks;
        occupiedByBlack = bPawns | bKing | bQueen | bKnights | bBishops | bRooks;
        Bitboard allOccupiedSpaces = occupiedByBlack | occupiedByWhite;
        string curRowString[8] = {"\033[0m 8", "\033[0m 7", "\033[0m 6", "\033[0m 5", "\033[0m 4", "\033[0m 3", "\033[0m 2", "\033[0m 1"};
        for(int row = 0; row < 8; row++){
            for(int col = 0; col < 8; col++){
                Bitboard square = (1ULL << (row*8+col));
                bool isSel = (selectedSquares & square) ? true : false;
                string bgColor = ((row+col)%2 == 0) ? "\033[48;2;138;95;23m\033[38;2;138;95;23m" : "\033[48;2;187;187;194m\033[38;2;187;187;194m";
                int tileOccupation = getTileOccupation(row, col);
                char pieceChar = getPieceCharacter(tileOccupation);
                string pieceString = (tileOccupation < 10) ? "\033[97m" : "\033[30m";
                pieceString += pieceChar;
                if(isSel == true){
                    bgColor = "\033[92m\033[102m";
                }
                cout << bgColor + "[" + pieceString + bgColor + "]";
            }
            cout << curRowString[row] << endl;
        }
        cout << "\033[0m A  B  C  D  E  F  G  H" << endl;
    }
    
    Bitboard dMovement(int square, Bitboard enemyLocations, Bitboard friendlyLocations){
        Bitboard validMoves = 0;
        Bitboard occupied = enemyLocations | friendlyLocations;
        Bitboard northeast = (1ULL << square);
        while((northeast & ~0x0101010101010101) && (northeast & ~0xFF00000000000000)){
            northeast <<= 7;
            if((northeast & 0x0101010101010101) || (northeast & 0xFF00000000000000)) break;
            validMoves |= northeast;
            if(northeast & occupied) break;
        }
        Bitboard northwest = (1ULL << square);
        while((northwest & ~0x8080808080808080) && (northwest & ~0xFF00000000000000)){
            northwest <<= 9;
            if((northwest & 0x8080808080808080) || (northwest & 0xFF00000000000000)) break;
            validMoves |= northwest;
            if(northwest & occupied) break;
        }
        Bitboard southeast = (1ULL << square);
        while((southeast & ~0x0101010101010101) && (southeast & ~0x00000000000000FF)){
            southeast >>= 7;
            if((southeast & 0x0101010101010101) || (southeast & 0xFF00000000000000)) break;
            validMoves |= southeast;
            if(southeast & occupied) break;
        }
        Bitboard southwest = (1ULL << square);
        while((southwest & ~0x8080808080808080) && (southwest & ~0x00000000000000FF)){
            southwest >>= 9;
            if((southwest & 0x8080808080808080) || (southwest & 0xFF00000000000000)) break;
            validMoves |= southwest;
            if(southwest & occupied) break;
        }
        return validMoves;
    }
    
    Bitboard vhMovement(int square, Bitboard enemyLocations, Bitboard friendlyLocations){
        Bitboard validMoves = 0;
        Bitboard occupied = enemyLocations | friendlyLocations;
        Bitboard left = (1ULL << square);
        while(left & ~0x0101010101010101){
            left >>= 1;
            validMoves |= left;
            if(left & occupied) break;
        }
        Bitboard right = (1ULL << square);
        while(right & ~0x8080808080808080){
            right <<= 1;
            validMoves |= right;
            if(right & occupied) break;
        }
        Bitboard up = (1ULL << square);
        while(up & ~0xFF00000000000000){
            up <<= 8;
            validMoves |= up;
            if(up & occupied) break;
        }
        Bitboard down = (1ULL << square);
        while(down & ~0x00000000000000FF){
            down >>= 8;
            validMoves |= down;
            if(down & occupied) break;
        }
        return validMoves;
    }
    
    void generateMovesets(){
        occupiedByWhite = wPawns | wKing | wQueen | wKnights | wBishops | wRooks;
        occupiedByBlack = bPawns | bKing | bQueen | bKnights | bBishops | bRooks;
        Bitboard allOccupiedSpaces = occupiedByBlack | occupiedByWhite;
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                Bitboard pieceValidMoves = 0;
                Bitboard tile = 1ULL << (row*8+col);
                if(tile & allOccupiedSpaces){
                    int curPiece = getTileOccupation(row, col);
                    int curColor = (curPiece < 10) ? 0 : 1;
                    Bitboard enemyLocations = (curColor == 0) ? occupiedByBlack : occupiedByWhite;
                    Bitboard friendlyLocations = (curColor == 0) ?  occupiedByWhite: occupiedByBlack;
                    if(curPiece == 1 || curPiece == 11){
                        int dir = (curColor == 0) ? -1 : 1;
                        //forward movement
                        Bitboard possMove = 1ULL << ((row+dir)*8+col);
                        if(!(possMove & allOccupiedSpaces)){
                            pieceValidMoves |= 1ULL << ((row+dir)*8+col);
                            Bitboard pawnDoubleMove = 1ULL << ((row+dir+dir)*8+col);
                            Bitboard curStartingRow = (curColor == 0) ? bPawnStartingRow : wPawnStartingRow;
                            if(!(pawnDoubleMove & allOccupiedSpaces) && (curStartingRow & tile)){
                                pieceValidMoves |= 1ULL << ((row+dir+dir)*8+col);
                            }
                            
                        }
                        for(int dir2 = -1; dir2 <= 1; dir2+=2){
                            possMove = 1ULL << ((row+dir)*8+(col+dir2));
                            if(enemyLocations & possMove){
                                pieceValidMoves |= 1ULL << ((row+dir)*8+(col+dir2));
                            }
                        }
                        Bitboard allCaptureMoves = enemyLocations & pieceValidMoves;
                        moveSetsUnfiltered[(row*8)+col] = allCaptureMoves | pieceValidMoves;
                        pieceValidMoves &= ~enemyLocations;
                        moveSetsCapt[(row*8)+col] = allCaptureMoves;
                        moveSets[(row*8)+col] = pieceValidMoves;
                    }
                    if(curPiece == 2 || curPiece == 12){
                        //up, up left, left, up right
                        int dirs[8] = {9,7,1,8,9,7,1,8};
                        Bitboard masks[8] = {0x0101010101010101, 0x8080808080808080, 0x8080808080808080, 0xFF00000000000000, 0x8080808080808080, 0x0101010101010101, 0x0101010101010101, 0x00000000000000FF};
                        Bitboard square = 1ULL << (row*8+col);
                        Bitboard validMoves = 0;
                        for(int dir = 0; dir < 8; dir++){
                            Bitboard tsquare = square;
                            Bitboard curMask = masks[dir];
                            if(tsquare & ~curMask){
                                if(dir > 3){
                                    tsquare >>= dirs[dir];
                                    validMoves |= tsquare;
                                } else{
                                    tsquare <<= dirs[dir];
                                    validMoves |= tsquare;
                                }
                            }
                        }
                        Bitboard allCaptureMoves = validMoves & enemyLocations;
                        moveSetsUnfiltered[row*8+col] = validMoves;
                        moveSetsCapt[row*8+col] = allCaptureMoves;
                        validMoves &= ~friendlyLocations;
                        validMoves &= ~allCaptureMoves;
                        moveSets[row*8+col] = validMoves;
                    }
                    if(curPiece == 3 || curPiece == 13){
                        Bitboard diagMvmt = dMovement(((row*8)+col), enemyLocations, friendlyLocations);
                        Bitboard verthorizontalMovement =vhMovement(((row*8)+col), enemyLocations, friendlyLocations);
                        Bitboard finalMvmt = diagMvmt | verthorizontalMovement;
                        moveSetsUnfiltered[row*8+col] = finalMvmt;
                        finalMvmt &= ~friendlyLocations;
                        Bitboard allCaptureMoves = finalMvmt & enemyLocations;
                        moveSetsCapt[row*8+col] = allCaptureMoves;
                        finalMvmt &= ~enemyLocations;
                        moveSets[row*8+col] = finalMvmt;
                    }
                    if(curPiece == 4 || curPiece == 14){
                        Bitboard diagMvmt = dMovement(((row*8)+col), enemyLocations, friendlyLocations);
                        moveSetsUnfiltered[row*8+col] = diagMvmt;
                        diagMvmt &= ~friendlyLocations;
                        Bitboard allCaptureMoves = diagMvmt & enemyLocations;
                        moveSetsCapt[row*8+col] = allCaptureMoves;
                        diagMvmt &= ~enemyLocations;
                        moveSets[row*8+col] = diagMvmt;
                    }
                    if(curPiece == 5 || curPiece == 15){
                        Bitboard validMoves = 0;
                        int dirs[8] = {15,17,6,10,10,6,17, 15};
                        Bitboard masks[8] = {0x8080808080808080, 0x0101010101010101, 0xC0C0C0C0C0C0C0C0, 0x0303030303030303, 0xC0C0C0C0C0C0C0C0, 0x0303030303030303, 0x8080808080808080, 0x0101010101010101};
                        Bitboard knightPos = 1ULL << (row*8+col);
                        for(int x = 0; x < 8; x++){
                            Bitboard tempSquare = knightPos;
                            Bitboard curMask = masks[x];
                            if(tempSquare & ~curMask){
                                if(x > 3){
                                    tempSquare <<= dirs[x];
                                    validMoves |= tempSquare;
                                } else{
                                    tempSquare >>= dirs[x];
                                    validMoves |= tempSquare;
                                }
                            }
                        }
                        moveSetsUnfiltered[row*8+col] = validMoves;
                        validMoves &= ~friendlyLocations;
                        Bitboard allCaptureMoves = validMoves & enemyLocations;
                        moveSetsCapt[row*8+col] = allCaptureMoves;
                        validMoves &= ~enemyLocations;
                        moveSets[row*8+col] = validMoves;
                    }
                    if(curPiece == 6 || curPiece == 16){
                        Bitboard verthorizontalMovement = vhMovement(((row*8)+col), enemyLocations, friendlyLocations);
                        moveSetsUnfiltered[row*8+col] = verthorizontalMovement;
                        verthorizontalMovement &= ~friendlyLocations;
                        Bitboard allCaptureMoves = verthorizontalMovement & enemyLocations;
                        moveSetsCapt[row*8+col] = allCaptureMoves;
                        verthorizontalMovement &= ~enemyLocations;
                        moveSets[row*8+col] = verthorizontalMovement;
                    }
                } else{
                    Bitboard tempBitboard = 0;
                    moveSets[row*8+col] = tempBitboard;
                }
                
            }
        }
    }
    
    int doPromotion(int color){
        array<int, 4> pieces = (color == 0) ? array<int, 4>{3,4,5,6} : array<int, 4>{13,14,15,16};
        scrnClear();
        cout << "What would you like to promote your pawn to?" << endl;
        for(int piece = 0; piece < pieces.size(); piece++){
            cout << getPieceCharacter(pieces[piece]) << "[" << to_string((piece+1)) << "]" << endl;
        }
        int chosenPiece;
        cin >> chosenPiece;
        if(chosenPiece < 5 && chosenPiece > 0){
            return pieces[chosenPiece-1];
        }
        return 11;
    }
    
    array<int, 10> getPieceAmtEach(){
        int amtWhitePawns = __builtin_popcountll(wPawns);
        int amtWhiteQueens = __builtin_popcountll(wQueen);
        int amtWhiteBishops = __builtin_popcountll(wBishops);
        int amtWhiteKnights = __builtin_popcountll(wKnights);
        int amtWhiteRooks = __builtin_popcountll(wRooks);
        int amtBlackPawns = __builtin_popcountll(bPawns);
        int amtBlackQueens = __builtin_popcountll(bQueen);
        int amtBlackBishops = __builtin_popcountll(bBishops);
        int amtBlackKnights = __builtin_popcountll(bKnights);
        int amtBlackRooks = __builtin_popcountll(bRooks);
        array<int, 10> pieceCounts = {amtWhitePawns, amtWhiteQueens, amtWhiteBishops, amtWhiteKnights, amtWhiteRooks, amtBlackPawns, amtBlackQueens, amtBlackBishops, amtBlackKnights, amtBlackRooks};
        return pieceCounts;
        
    }
    
    array<float, 2> calculateMaterial(array<int,10> pieceCounts){
        int pieceMaterialVals[10] = {100,900,300,330,500,100,900,300,330,500};
        array<float, 2> values;
        int whiteMaterial = 0;
        int blackMaterial = 0;
        for(int x = 0; x < 10; x++){
            if(x > 4){
                blackMaterial += pieceCounts[x]*pieceMaterialVals[x];
            } else{
                whiteMaterial += pieceCounts[x]*pieceMaterialVals[x];
            }
        }
        values[0] = whiteMaterial;
        values[1] = blackMaterial;
        return values;
    }
    
    int calculateBoardControl(int color){
        int blackControl = 0;
        int whiteControl = 0;
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                Bitboard currentLocation = 1ULL << ((row*8+col));
                int tileOccupation = getTileOccupation(row, col);
                int curColor = (tileOccupation > 10) ? 1 : 0;
                Bitboard currentMoveset = moveSetsUnfiltered[row*8+col];
                int amtMoves = __builtin_popcountll(currentMoveset);
                if(amtMoves > 0){
                    if(currentLocation & occupiedByBlack){
                        curColor = 1;
                    }
                    Bitboard enemyLocations = (color == 0) ? occupiedByBlack : occupiedByWhite;
                    Bitboard friendlyLocations = (color == 0) ?  occupiedByWhite: occupiedByBlack;
                    currentMoveset &= ~friendlyLocations;
                    currentMoveset &= ~enemyLocations;
                    Bitboard inCenter = currentMoveset & centerSpaces;
                    Bitboard aroundCenter = currentMoveset & outCenterSpaces;
                    currentMoveset &= ~centerSpaces;
                    currentMoveset &= ~outCenterSpaces;
                    int amtInCenter = __builtin_popcountll(inCenter);
                    int amtOutCenter = __builtin_popcountll(aroundCenter);
                    int outerBoard = __builtin_popcountll(currentMoveset);
                    if(curColor == 0){
                        whiteControl += amtInCenter + amtOutCenter + outerBoard;
                    } else{
                        blackControl += amtInCenter + amtOutCenter + outerBoard;
                    }
                }
            }
        }
        //cout << "whiteControl: "<< whiteControl << endl;
        //cout << "blackControl: " << blackControl << endl;
        if(color == 0){
            return (whiteControl-blackControl)*100;
        } else{
            return (blackControl-whiteControl)*100;
        }
    }
    
    float totalDefenseVal(int color){
        array<Bitboard*, 5> friendlyBitboards = {nullptr, nullptr, nullptr, nullptr, nullptr};
        float whiteDefense = 0;
        float blackDefense = 0;
        int matVals[] = {1,9,3,3,5};
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                int tileOccupation = getTileOccupation(row, col);
                int curColor = (tileOccupation > 10) ? 1 : 0;
                Bitboard currentMove = moveSetsUnfiltered[row*8+col];
                if(curColor == 0){
                    array<Bitboard*, 5> tempBitboard = {&wPawns, &wQueen, &wBishops, &wKnights, &wRooks};
                    friendlyBitboards = tempBitboard;
                } else{
                    array<Bitboard*, 5> tempBitboard = {&bPawns, &bQueen, &bBishops, &bKnights, &bRooks};
                    friendlyBitboards = tempBitboard;
                }
                for(int x = 0; x < 5; x++){
                    Bitboard curBitboard = *(friendlyBitboards[x]);
                    Bitboard overlapping = currentMove & curBitboard;
                    int defenseVal = __builtin_popcountll(overlapping);
                    if(curColor == 0){
                        whiteDefense += defenseVal*matVals[x];
                    } else{
                        blackDefense += defenseVal*matVals[x];
                    }
                }
            }
        }
        if(color == 0){
            return (whiteDefense-blackDefense)*50;
        } else{
            return (blackDefense-whiteDefense)*50;
        }
    }
    
    int totalThreatVal(int color){
        array<Bitboard*, 5> enemyBitboards = {nullptr, nullptr, nullptr, nullptr, nullptr};
        float whiteThreaten = 0;
        float blackThreaten = 0;
        int matVals[] = {1,9,3,3,5};
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                int tileOccupation = getTileOccupation(row, col);
                int curColor = (tileOccupation > 10) ? 1 : 0;
                Bitboard currentMove = moveSetsUnfiltered[row*8+col];
                if(curColor == 0){
                    array<Bitboard*, 5> tempBitboard = {&wPawns, &wQueen, &wBishops, &wKnights, &wRooks};
                    enemyBitboards = tempBitboard;
                } else{
                    array<Bitboard*, 5> tempBitboard = {&bPawns, &bQueen, &bBishops, &bKnights, &bRooks};
                    enemyBitboards = tempBitboard;
                }
                for(int x = 0; x < 5; x++){
                    Bitboard curBitboard = *(enemyBitboards[x]);
                    Bitboard overlapping = currentMove & curBitboard;
                    int threatVal = __builtin_popcountll(overlapping);
                    if(curColor == 0){
                        whiteThreaten += threatVal*matVals[x];
                    } else{
                        blackThreaten += threatVal*matVals[x];
                    }
                }
            }
        }
        if(color == 0){
            return (whiteThreaten-blackThreaten)*50;
        } else{
            return (blackThreaten-whiteThreaten)*50;
        }
    }
    
    int evalPawnProgression(int color){
        int whitePawnProgressionVal = 0;
        int blackPawnProgressionVal = 0;
        Bitboard wLO = wPawns & whitePawnAdvanceOne;
        Bitboard wLT = wPawns & whitePawnAdvanceTwo;
        Bitboard bLO = bPawns & blackPawnAdvanceOne;
        Bitboard bLT = bPawns & blackPawnAdvanceTwo;
        whitePawnProgressionVal += __builtin_popcountll(wLO) + (__builtin_popcountll(wLT)*2);
        blackPawnProgressionVal += __builtin_popcountll(bLO) + (__builtin_popcountll(bLT)*2);
        if(color == 0){
            return whitePawnProgressionVal-blackPawnProgressionVal;
        } else{
            return blackPawnProgressionVal-whitePawnProgressionVal;
        }
    }
    
    int evalMobility(int color){
        int mobilityFriendly = 0;
        int mobilityEnemy = 0;
        Bitboard friendlyLocations = (color == 0) ? occupiedByWhite : occupiedByBlack;
        Bitboard enemyLocations = (color == 0) ? occupiedByBlack : occupiedByWhite;
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                Bitboard currentMoveset = moveSets[row*8+col];
                int amtMovesCMS = __builtin_popcountll(currentMoveset);
                if(amtMovesCMS > 0){
                    Bitboard square = 1ULL << (row*8+col);
                    bool isFriendlyPiece = (square & friendlyLocations) ? true : false;
                    currentMoveset &= ~friendlyLocations;
                    currentMoveset &= ~enemyLocations;
                    int amtMovesFiltered = __builtin_popcountll(currentMoveset);
                    if(isFriendlyPiece == true){
                        mobilityFriendly += amtMovesFiltered;
                    } else{
                        mobilityEnemy += amtMovesFiltered;
                    }
                }   
            }
        }
        return (mobilityFriendly-mobilityEnemy)*50;
    }
    
    int kingProtectionVal(int color){
        int kingProtectionValFriendly = 0;
        int kingProtectionValEnemy = 0;
        Bitboard friendlyKingLocation = (color == 0) ? 1ULL << (wKing) : 1ULL << (bKing);
        Bitboard enemyKingLocation = (color == 1) ? 1ULL << (wKing) : 1ULL << (bKing);
        Bitboard friendlyLocations = (color == 0) ? 1ULL << (occupiedByWhite) : 1ULL << (occupiedByBlack);
        Bitboard enemyLocations = (color == 0) ? 1ULL << (occupiedByBlack) : 1ULL << (occupiedByWhite);
        for(int row = 7; row >= 0; row--){
            for(int col = 0; col < 8; col++){
                Bitboard curSquare = 1ULL << (row*8+col);
                int isFriendlyPiece = (friendlyLocations & curSquare) ? 0 : 1;
                Bitboard curMoveset = moveSetsUnfiltered[row*8+col];
                if(isFriendlyPiece == 0){
                    if(curMoveset & friendlyKingLocation){
                        kingProtectionValFriendly += 50;
                    }
                } else{
                    if(curMoveset & enemyKingLocation){
                        kingProtectionValEnemy += 50;
                    }
                }
            }
        }
        return (kingProtectionValFriendly-kingProtectionValEnemy)*10;
    }
    
    int evalBoard(int color){
        array<int,10> pieceCounts = getPieceAmtEach();
        array<float, 2> matsPerColor = calculateMaterial(pieceCounts);
        int totMaterial = (color == 0) ? matsPerColor[0] - matsPerColor[1] : matsPerColor[1] - matsPerColor[0];
        int boardControlVal = calculateBoardControl(color);
        int pawnProgressionVal = evalPawnProgression(color);
        int boardThreatVal = totalThreatVal(color);
        float boardDefenseVal = totalDefenseVal(color);
        int mobility = evalMobility(color);
        int kingProtection = kingProtectionVal(color);
        return ((totMaterial*20)+(boardDefenseVal/3)+(boardControlVal/3)+(mobility/5)+(boardThreatVal/2)+kingProtection*5);
    }
    
    void doMove(Bitboard moveMesh, int endPos[2], int startPos[2]){
        Bitboard endPosition = 1ULL << (endPos[0]*8+endPos[1]);
        Bitboard startPosition = 1ULL << (startPos[0]*8+startPos[1]);
        Bitboard* pieceBitboards[]{
            nullptr, &wPawns, &wKing, &wQueen, &wBishops, &wKnights, &wRooks,
            nullptr, nullptr, nullptr, nullptr,
            &bPawns, &bKing, &bQueen, &bBishops, &bKnights, &bRooks
        };
        
        if (!(moveMesh & endPosition)) {
            cout << "problem" << endl;
            this_thread::sleep_for(chrono::milliseconds(5000));
            getPlayerInput();
            return;
        }
        
        int startPiece = getTileOccupation(startPos[0],startPos[1]);
        int endPiece = getTileOccupation(endPos[0],endPos[1]);
        
        if(endPiece > 0){
            *(pieceBitboards[endPiece]) &= ~endPosition;
        }
        if((endPosition & wPawnPromotionRow) && startPiece == 1){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            int promotedPiece = doPromotion(0);
            *(pieceBitboards[promotedPiece]) |= endPosition;
        } else if((endPosition & bPawnPromotionRow) && startPiece == 11){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            int promotedPiece = doPromotion(1);
            *(pieceBitboards[promotedPiece]) |= endPosition;
        } else{
            *(pieceBitboards[startPiece]) &= ~startPosition;
            *(pieceBitboards[startPiece]) |= endPosition;
        }
        generateMovesets();
    }
    
    void doAiMove(Bitboard moveMesh, int startPos, int endPos, int startPiece, int endPiece){
        Bitboard startPosition = 1ULL << (startPos);
        Bitboard endPosition = 1ULL << (endPos);
        Bitboard* pieceBitboards[]{
            nullptr, &wPawns, &wKing, &wQueen, &wBishops, &wKnights, &wRooks,
            nullptr, nullptr, nullptr, nullptr,
            &bPawns, &bKing, &bQueen, &bBishops, &bKnights, &bRooks
        };
        if (!(moveMesh & endPosition)) {
            return;
        }
        if(endPiece > 0){
            *(pieceBitboards[endPiece]) &= ~endPosition;
        }
        if((endPosition & wPawnPromotionRow) && startPiece == 1){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            int promotedPiece;
            *(pieceBitboards[13]) |= endPosition;
        } else if((endPosition & bPawnPromotionRow) && startPiece == 11){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            *(pieceBitboards[13]) |= endPosition;
        } else{
            if(startPiece == 0){
                *(pieceBitboards[endPiece]) &= ~startPosition;
                *(pieceBitboards[endPiece]) |= endPosition;
            } else{
                *(pieceBitboards[startPiece]) &= ~startPosition;
                *(pieceBitboards[startPiece]) |= endPosition;
            }
        }
        generateMovesets();
        //cout << "got to end of doaimove" << endl;
    }
    
};

Chessboard board;

void scrnClear(){
    system("clear");
}

char getPieceCharacter(int pieceNum){
    array<char, 7> pieceChar = {' ', 'P', 'K', 'Q', 'B', 'H', 'R'};
    int shiftedPieceNum = (pieceNum > 10) ? (pieceNum - 10) : pieceNum;
    return pieceChar[shiftedPieceNum];
}


vector<int> convertPlayerInput(string playerIn){
    vector<int> temp;
    vector<char> letVal = {'a','b','c','d','e','f','g','h'};
    char baseLet = playerIn[0];
    int row = playerIn[1] - '0';
    row = 8-row;
    temp.push_back(row);
    for(int x = 0; x < letVal.size(); x++){
        if(letVal[x] == baseLet){
            temp.push_back(x);
        }
    }
    return temp;
}



void confirmMove(int row, int col){
    Bitboard curPiece = 1ULL << (row*8+col);
    Bitboard curMovesetNormal = board.moveSets[((row*8)+col)];
    Bitboard curMovesetCapture = board.moveSetsCapt[((row*8)+col)];
    Bitboard finalBitboard = curMovesetNormal | curPiece | curMovesetCapture;
    board.printBoard(finalBitboard);
    //cout << "Index of Tile = " << row*8+col << endl;
    string pIn;
    cin >> pIn;
    vector<int> playerIn = convertPlayerInput(pIn);
    int startPos[2] = {row, col};
    int endPos[2] = {playerIn[0],playerIn[1]};
    Bitboard square = 1ULL << ((endPos[0]*8+endPos[1]));
    if(curMovesetNormal & square){
        Bitboard curMoveset = curMovesetNormal;
        int curWhiteEval = board.evalBoard(0);
        int curBlackEval = board.evalBoard(1);
        board.prevWhiteEval = curWhiteEval;
        board.prevBlackEval = curBlackEval;
        board.doMove(curMoveset, endPos, startPos);
        if(aiEnabled){
            int didAiMove = beginAiMove();
            if(didAiMove == 0){
                //cout << "beep" << endl;
                //this_thread::sleep_for(chrono::milliseconds(5000));
            }
        }
        getPlayerInput();
    } else{
        Bitboard curMoveset = curMovesetCapture;
        board.doMove(curMoveset, endPos, startPos);
        if(aiEnabled){
            int didAiMove = beginAiMove();
            if(didAiMove == 0){
                //cout << "beep" << endl;
                //this_thread::sleep_for(chrono::milliseconds(5000));
            }
        }
        getPlayerInput();
    }
    
}

void getPlayerInput(){
    Bitboard emptyBoard = 0;
    int curWhiteEval = board.evalBoard(0);
    int curBlackEval = board.evalBoard(1);
    board.printBoard(emptyBoard);
    cout << "Black prev + cur eval: [" << board.prevBlackEval << "," << curBlackEval << "]" << endl;
    cout << "White prev + cur eval: [" << board.prevWhiteEval << "," << curWhiteEval << "]" << endl;
    string pIn;
    cin >> pIn;
    vector<int> playerIn = convertPlayerInput(pIn);
    confirmMove(playerIn[0],playerIn[1]);
}


int miniMax(int depth, int maxDepth, bool isMax, int &maximum, int &minimum){
    // cout << "minimax" << endl;
    int color = (isMax == true) ? 0 : 1;
    int initBoardEvalScore = board.evalBoard(color);
    if(depth == maxDepth){
        return initBoardEvalScore;
    } else{
        int kingPopCountW = __builtin_popcountll(board.wKing);
        int kingPopCountB = __builtin_popcountll(board.bKing);
        if(kingPopCountW < 1){
            if(color == 1){
                return 9999999;
            } else{
                return -999999;
            }
        } else if(kingPopCountB < 1){
            if(color == 0){
                return 9999999;
            } else{
                return -999999;
            }
        }
    }
    
    //set the occupation of friendly pieces
    Bitboard curOccupation = (isMax == true) ? board.occupiedByBlack : board.occupiedByWhite;
    //store what the next color to call will be
    bool nextColor = (isMax == true) ? false : true;
    // cout << "e" << endl;
    // cout << "a" << endl;
    int tempEval = (isMax == true) ? -999999 : 999999;
    // cout << "sports" << endl;
    for(int row = 7; row >= 0; row--){
        for(int col = 0; col < 8; col++){
            // cout << "its in the game" << endl;
            // cout << "beep" << endl;
            int selectedPiece = board.getTileOccupation(row, col);
            // cout << selectedPiece << endl;
            // cout << "bop" << endl;
            Bitboard selectedPieceLocation = 1ULL << ((row*8)+col);
            //if its a friendly piece
            if(selectedPieceLocation & curOccupation){
                //get current move mesh
                Bitboard currentMove = board.moveSetsUnfiltered[row*8+col];
                //parse all moves of moveMesh
                for(int moveRow = 7; moveRow >= 0; moveRow--){
                    for(int moveCol = 0; moveCol < 8; moveCol++){
                        //cout << "boop" << endl;
                        int endingPieceVal = board.getTileOccupation(moveRow, moveCol);
                        Chessboard initBoardState = board;
                        board.doAiMove(currentMove, (row*8+col), (moveRow*8+moveCol), selectedPiece, endingPieceVal);
                        int eval = miniMax(depth+1, maxDepth, nextColor, maximum, minimum);
                        if(debugMiniMax){
                            scrnClear();
                            cout << "Min or Max: " << isMax << endl;
                            cout << "Current Eval: " << eval << endl;
                            cout << "[" << row << ", " << col << "]" << endl << "[" << moveRow << ", " << moveCol << "]" << endl;
                            string printColor = (color == 1) ? "black" : "white";
                            cout << "Color: " << printColor << endl;
                            cout << "Maximum: " << maximum << endl;
                            cout << "Minimum: " << minimum << endl;
                            cout << "Depth: " << depth << "/" << maxDepth << endl;
                            // this_thread::sleep_for(chrono::milliseconds(50));
                        }
                        board = initBoardState;
                        if(isMax == true){
                            if(debugMiniMax){
                                cout << "maximuzing" << endl;
                            }
                            tempEval = max(tempEval, eval);
                            maximum = max(maximum, tempEval);
                        }   else{
                            if(debugMiniMax){
                                cout << "minimizing" << endl;
                                // this_thread::sleep_for(chrono::milliseconds(500));
                            }
                            tempEval = min(tempEval, eval);
                            minimum = min(minimum, tempEval);
                        }
                        if(minimum <= maximum){
                            if(debugMiniMax){
                                cout << "returning tempeval" << endl;
                            }
                            return tempEval;
                        }
                    }
                }
            }
        }
    }
    return tempEval;
}

/*int miniMax(Chessboard curBoard, int depth, int maxDepth, bool isMax){
    Bitboard curOccuption = (isMax == true) ? curBoard.occupiedByBlack : curBoard.occupiedByWhite;
    bool nextType = (isMax == true) ? false : true;
    for(int row = 7; row >= 0; row--){
        for(int col = 0; col < 8; col++){
            Bitboard curMoveset = curBoard.moveSets[row*8+col];
            if(curMoveset & curOccuption){
                for(int tempRow = 7; tempRow >= 0; tempRow--){
                    for(int tempCol = 0; tempCol < 8; tempCol++){
                        Chessboard tempCurBoard;
                        tempCurBoard = curBoard;
                        Bitboard tile = 1ULL << ((tempRow*8+tempCol));
                        if(curMoveset & tile){
                            int startingPiece = tempCurBoard.getTileOccupation(row, col);
                            int endingPiece = tempCurBoard.getTileOccupation(tempRow, tempCol);
                            tempCurBoard.doAiMove(curMoveset, row*8+col, tempRow*8+tempCol, startingPiece, endingPiece);
                            float eval = miniMax(tempCurBoard, depth+1, curMaxDepth, nextType);
                            
                            if(eval > curMaxEval){
                                curMaxEval = eval;
                                moveId = row*8+col;
                                endPos |= 1ULL << ((tempRow*8+tempCol));
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}*/

array<int, 4> bestMove(){
    int moveId = 0;
    int endPos = 0;
    int startPiece = 0;
    int endPiece = 0;
    int curBoardPop = __builtin_popcountll(board.occupiedByWhite);
    //cout << curBoardPop << endl;
    curBoardPop += __builtin_popcountll(board.occupiedByBlack);
    int curMaxDepth = (curBoardPop < 12) ? 100 : 75;
    Bitboard blackBoard = board.occupiedByBlack;
    Bitboard whiteBoard = board.occupiedByWhite;
    int initMax = 9999999;
    int initMin = -9999999;
    int curMaxEval = -999999;
    for(int row = 7; row >= 0; row--){
        for(int col = 0; col < 8; col++){
            Bitboard curSquare = 1ULL << ((row*8+col));
            if(curSquare & blackBoard){
                Bitboard curMoveset = board.moveSetsUnfiltered[row*8+col];
                curMoveset &= ~curSquare;
                curMoveset &= ~blackBoard;
                //cout << "andBlack" << endl;
                for(int tempRow = 7; tempRow >= 0; tempRow--){
                    for(int tempCol = 0; tempCol < 8; tempCol++){
                        Bitboard tile = 1ULL << ((tempRow*8+tempCol));
                        if(curMoveset & tile){
                            int startingPiece = board.getTileOccupation(row, col);
                            int endingPiece = board.getTileOccupation(tempRow, tempCol);
                            Chessboard savedBoardState = board;
                            board.doAiMove(curMoveset, row*8+col, tempRow*8+tempCol, startingPiece, endingPiece);
                            float eval = miniMax(0, curMaxDepth, false, initMax, initMin);
                            board = savedBoardState;
                            if(eval > curMaxEval){
                                endPos = 0;
                                curMaxEval = eval;
                                // cout << "updated maxeval" << endl;
                                startPiece = board.getTileOccupation(row,col);
                                endPiece = board.getTileOccupation(tempRow,tempCol);
                                moveId = row*8+col;
                                endPos = tempRow*8+tempCol;
                            }
                        }
                    }
                }
            }
        }
    }
    // this_thread::sleep_for(chrono::milliseconds(5000));
    
    //cout << "returning move id: " << moveId << endl;
    return {moveId, endPos, endPiece, startPiece};
}


int beginAiMove(){
    int beginningBoardEvalB = board.evalBoard(1);
    int beginningBoardEvalW = board.evalBoard(0);
    board.prevBlackEval = beginningBoardEvalB;
    board.prevWhiteEval = beginningBoardEvalW;
    Chessboard boardBackup = board;
    array<int, 4> bestPossible = bestMove();
    //cout << "??" << endl;
    // board = boardBackup;
    board.generateMovesets();
    Bitboard moveMesh = board.moveSetsUnfiltered[bestPossible[0]];
    //cout << bestPossible[0] << endl << bestPossible[1] << endl << bestPossible[2] << endl << bestPossible[3] << endl;
    //cout << "aljskfdlasjdfkjasdlfj" << endl;
    board.doAiMove(moveMesh, bestPossible[0], bestPossible[1], bestPossible[2], bestPossible[3]);
    //cout << "beginAiMove is done" << endl;
    int endingBoardEval = board.evalBoard(1);
    //cout << "starting eval: " << beginningBoardEval << endl;
    //cout << "ending eval: " << endingBoardEval << endl;
    //this_thread::sleep_for(chrono::milliseconds(5000));
    return 0;
}


int main()
{
    //board.wKnights |= 1ULL << (4*8+2);
    board.generateMovesets();
    getPlayerInput();
    
    return 0;
}