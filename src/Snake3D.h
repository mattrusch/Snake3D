// Snake3D.h

#pragma once

#include <DirectXMath.h>

namespace Snake
{
    class GamePiece
    {
    public:
        GamePiece() = default;
        ~GamePiece() = default;

        DirectX::XMVECTOR mColor;
        int               mRemainingTicks;
        GamePiece*        mNext;
    };

    constexpr size_t NumPiecesX = 64;
    constexpr size_t NumPiecesY = 64;
    constexpr size_t NumPiecesZ = 64;
    constexpr size_t NumGamePieces = NumPiecesX * NumPiecesY * NumPiecesZ;

    class GameBoard
    {
    public:
        GameBoard() = default;
        ~GameBoard() = default;

        void Init();

        DirectX::XMVECTOR GetPosition(int xBlock, int yBlock, int zBlock) const;
        const GamePiece* GetGamePiece(int xBlock, int yBlock, int zBlock) const;
        void PlaceGamePiece(int xBlock, int yBlock, int zBlock, const DirectX::XMVECTOR& color, int remainingTicks);

    private:
        GamePiece  mGamePiecePool[NumGamePieces];   // Pool of all gamepieces, contains enough to fill board completely
        GamePiece* mGamePieces[NumGamePieces];      // Locations on the board; can point to a game piece or be null
        GamePiece* mGamePieceFreeList;              // Allocation convenience

        float      mBoardWorldScale[3];             // Size of the board along world space axes

        GamePiece* AllocGamePiece();
        void FreeGamePiece(GamePiece*);
    };

} // namespace Snake
