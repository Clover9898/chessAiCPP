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

bool gameActive = true;

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
        
        //board information
        array<int, 17> pieceMaterials = {0, 1,999,9,3,3,5,0,0,0,0,1,999,9,3,3,5};
        
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
                            possMove = 0;
                            possMove = 1ULL << ((row+dir)*8+(col+dir2));
                            if(enemyLocations & possMove){
                                pieceValidMoves |= 1ULL << ((row+dir)*8+(col+dir2));
                            }
                        }
                        Bitboard allCaptureMoves = enemyLocations & pieceValidMoves;
                        pieceValidMoves &= ~enemyLocations;
                        moveSetsCapt[(row*8)+col] = allCaptureMoves;
                        moveSetsUnfiltered[(row*8)+col] = allCaptureMoves | pieceValidMoves;
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
    
    array<int, 2> calculateMaterial(array<int,10> pieceCounts){
        int pieceMaterialVals[10] = {1,9,3,3,5,1,9,3,3,5};
        array<int, 2> values;
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
        cout << "whiteControl: "<< whiteControl << endl;
        cout << "blackControl: " << blackControl << endl;
        if(color == 0){
            return whiteControl-blackControl;
        } else{
            return blackControl-whiteControl;
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
            return (whiteDefense-blackDefense)/2;
        } else{
            return (blackDefense-whiteDefense)/2;
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
            return (whiteThreaten-blackThreaten)/2;
        } else{
            return (blackThreaten-whiteThreaten)/2;
        }
    }
    
    void evalBoard(int color){
        array<int,10> pieceCounts = getPieceAmtEach();
        array<int, 2> matsPerColor = calculateMaterial(pieceCounts);
        for(int x = 0; x < 10; x++){
            cout << pieceCounts[x] << endl;
        }
        int totMaterial = (color == 0) ? matsPerColor[0] - matsPerColor[1] : matsPerColor[1] - matsPerColor[0];
        int boardControlVal = calculateBoardControl(color);
        float boardDefenseVal = totalDefenseVal(color);
        cout << totMaterial << endl;
        cout << boardControlVal << endl;
        cout << boardDefenseVal << endl;
        
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
        getPlayerInput();
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
        *(pieceBitboards[endPiece]) &= ~endPosition;
        if((endPosition & wPawnPromotionRow) && startPiece == 1){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            int promotedPiece;
            *(pieceBitboards[13]) |= endPosition;
        } else if((endPosition & bPawnPromotionRow) && startPiece == 11){
            *(pieceBitboards[startPiece]) &= ~startPosition;
            *(pieceBitboards[13]) |= endPosition;
        } else{
            *(pieceBitboards[startPiece]) &= ~startPosition;
            *(pieceBitboards[startPiece]) |= endPosition;
        }
        
        
        
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
    string pIn;
    cin >> pIn;
    vector<int> playerIn = convertPlayerInput(pIn);
    int startPos[2] = {row, col};
    int endPos[2] = {playerIn[0],playerIn[1]};
    Bitboard square = 1ULL << ((endPos[0]*8+endPos[1]));
    if(curMovesetNormal & square){
        Bitboard curMoveset = curMovesetNormal;
        board.doMove(curMoveset, endPos, startPos);
    } else{
        Bitboard curMoveset = curMovesetCapture;
        board.doMove(curMoveset, endPos, startPos);
    }
    
}

void getPlayerInput(){
    Bitboard emptyBoard = 0;
    board.printBoard(emptyBoard);
    string pIn;
    cin >> pIn;
    vector<int> playerIn = convertPlayerInput(pIn);
    confirmMove(playerIn[0],playerIn[1]);
}


int miniMax(Chessboard curBoard, int depth, int maxDepth, bool isMax){
    //set the occupation of friendly pieces
    Bitboard curOccupation = (isMax == true) ? curBoard.occupiedByBlack : curBoard.occupiedByWhite;
    //store what the next color to call will be
    bool nextColor = (isMax == true) ? false : true;
    //parse rows (up|down)
    for(int row = 7; row >= 0; row--){
        //parse columbs (left|right)
        for(int col = 0; col < 8; col++){
            int selectedPiece = getTileOccupation(row, col);
            Bitboard selectedPieceLocation = 1ULL << ((row*8)+col);
            //if its a friendly piece
            if(selectedPieceLocation & curOccuption){
                //get current move mesh
                Bitboard currentMove = board.moveSetsUnfiltered[row*8+col];
                //parse all moves of moveMesh
                for(int moveRow = 7; moveRow >= 0; moveRow--){
                    for(int moveCol = 0; moveCol < 8; moveCol++){
                        
                    }
                }
            }
        }
    }
    
    return 0;
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

int bestMove(Chessboard curBoard){
    int moveId = 0;
    Bitboard endPos = 0;
    int curBoardPop = __builtin_popcountll(curBoard.allOccupiedSpaces);
    int curMaxDepth = (curBoardPop < 12) ? 6 : 10;
    float curMaxEval = -999999;
    for(int row = 7; row >= 0; row--){
        for(int col = 0; col < 8; col++){
            Bitboard curMoveset = curBoard.moveSets[row*8+col];
            if(curMoveset & curBoard.occupiedByBlack){
                for(int tempRow = 7; tempRow >= 0; tempRow--){
                    for(int tempCol = 0; tempCol < 8; tempCol++){
                        Chessboard tempCurBoard;
                        tempCurBoard = curBoard;
                        Bitboard tile = 1ULL << ((tempRow*8+tempCol));
                        if(curMoveset & tile){
                            int startingPiece = tempCurBoard.getTileOccupation(row, col);
                            int endingPiece = tempCurBoard.getTileOccupation(tempRow, tempCol);
                            tempCurBoard.doAiMove(curMoveset, row*8+col, tempRow*8+tempCol, startingPiece, endingPiece);
                            float eval = miniMax(tempCurBoard, 0, curMaxDepth, false);
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
    
    
    return moveId;
}


int beginAiMove(){
    Chessboard boardCopy;
    boardCopy = board;
    bestMove(boardCopy);
    return 0;
}


int main()
{
    //board.wKnights |= 1ULL << (4*8+2);
    board.generateMovesets();
    board.evalBoard(0);
    //getPlayerInput();
    
    return 0;
}