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
        DirectX::XMVECTOR mPosition;
        int               mRemainingTicks;
        GamePiece*        mNext;
    };

    constexpr size_t NumPiecesX = 16;
    constexpr size_t NumPiecesY = 16;
    constexpr size_t NumPiecesZ = 16;
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
        const GamePiece* const* GetGamePieces(size_t* outNumGamePieces) const;

    private:
        GamePiece  mGamePiecePool[NumGamePieces];   // Pool of all gamepieces, contains enough to fill board completely
        GamePiece* mGamePieces[NumGamePieces];      // Locations on the board; can point to a game piece or be null
        GamePiece* mGamePieceFreeList;              // Allocation convenience

        float      mBoardWorldScale[3] = {static_cast<float>(NumPiecesX), static_cast<float>(NumPiecesY), static_cast<float>(NumPiecesZ)};        // Size of the board along world space axes

        GamePiece* AllocGamePiece();
        void FreeGamePiece(GamePiece*);
    };

} // namespace Snake
