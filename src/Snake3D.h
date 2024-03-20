// Snake3D.h

#pragma once

#include <DirectXMath.h>

namespace Snake
{
    enum class GamePieceType
    {
        SnakeBody,
        PowerUp,
        Wall
    };

    class GamePiece
    {
    public:
        GamePiece() = default;
        ~GamePiece() = default;

        DirectX::XMVECTOR mColor;
        DirectX::XMVECTOR mPosition;
        GamePiece*        mNext;
        int               mRemainingTicks;
        GamePieceType     mGamePieceType;
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
        void Reset();

        DirectX::XMVECTOR GetPosition(int xBlock, int yBlock, int zBlock) const;
        void GetBlockCoords(const DirectX::XMVECTOR& position, int& xBlockOut, int& yBlockOut, int& zBlockOut) const;
        const GamePiece* GetGamePiece(int xBlock, int yBlock, int zBlock) const;
        GamePiece* GetGamePiece(int xBlock, int yBlock, int zBlock);
        void PlaceGamePiece(int xBlock, int yBlock, int zBlock, const DirectX::XMVECTOR& color, int remainingTicks, GamePieceType gamePieceType);
        void RemoveGamePiece(int xBlock, int yBlock, int zBlock);
        const GamePiece* const* GetGamePieces(size_t* outNumGamePieces) const;

    private:
        // TODO: Turn GamePiecePool and mGamePieceFreeList (as well as corresponding alloc / free) into a pooled resource class
        GamePiece  mGamePiecePool[NumGamePieces];   // Pool of all gamepieces, contains enough to fill board completely
        GamePiece* mGamePieces[NumGamePieces];      // Locations on the board; can point to a game piece or be null
        GamePiece* mGamePieceFreeList;              // Allocation convenience

        float      mBoardWorldScale[3] = {static_cast<float>(NumPiecesX), static_cast<float>(NumPiecesY), static_cast<float>(NumPiecesZ)};        // Size of the board along world space axes

        GamePiece* AllocGamePiece();
        void FreeGamePiece(GamePiece*);
    };

} // namespace Snake
