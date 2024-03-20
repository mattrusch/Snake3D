// Snake3D.cpp

#include "Snake3D.h"
#include <cassert>

template<typename T, size_t N> constexpr size_t ArraySize(T(&)[N])
{
    return N;
}

namespace Snake
{
    static size_t CalcIndex(int xBlock, int yBlock, int zBlock)
    {
        return xBlock + yBlock * NumPiecesX + zBlock * NumPiecesX * NumPiecesY;
    }

    void GameBoard::Init()
    {
        // Initialize pool
        size_t gamePieceArraySize = ArraySize(mGamePiecePool);
        for (size_t i = 0; i < gamePieceArraySize - 1; i++)
        {
            // Set up linked list
            mGamePiecePool[i].mNext = &mGamePiecePool[i + 1];
        }

        // Set up final item in pool
        size_t finalIndex = gamePieceArraySize - 1;
        mGamePiecePool[finalIndex].mNext = nullptr;

        mGamePieceFreeList = mGamePiecePool;
    }

    void GameBoard::Reset()
    {
        // Reset pool to initial state
        Init();

        for (size_t i = 0; i < NumGamePieces; i++)
        {
            mGamePieces[i] = nullptr;
        }
    }

    DirectX::XMVECTOR GameBoard::GetPosition(int xBlock, int yBlock, int zBlock) const
    {
        return DirectX::XMVectorSet(
            mBoardWorldScale[0] / static_cast<float>(NumPiecesX) * static_cast<float>(xBlock),
            mBoardWorldScale[1] / static_cast<float>(NumPiecesY) * static_cast<float>(yBlock),
            mBoardWorldScale[2] / static_cast<float>(NumPiecesZ) * static_cast<float>(zBlock),
            1.0f);
    }

    void GameBoard::GetBlockCoords(const DirectX::XMVECTOR& position, int& xBlockOut, int& yBlockOut, int& zBlockOut) const
    {
        float blockSizeX = mBoardWorldScale[0] / static_cast<float>(NumPiecesX);
        float blockSizeY = mBoardWorldScale[1] / static_cast<float>(NumPiecesY);
        float blockSizeZ = mBoardWorldScale[2] / static_cast<float>(NumPiecesZ);

        xBlockOut = static_cast<int>(DirectX::XMVectorGetX(position) / blockSizeX);
        yBlockOut = static_cast<int>(DirectX::XMVectorGetY(position) / blockSizeY);
        zBlockOut = static_cast<int>(DirectX::XMVectorGetZ(position) / blockSizeZ);
    }

    const GamePiece* GameBoard::GetGamePiece(int xBlock, int yBlock, int zBlock) const
    {
        return mGamePieces[CalcIndex(xBlock, yBlock, zBlock)];
    }

    GamePiece* GameBoard::GetGamePiece(int xBlock, int yBlock, int zBlock)
    {
        return mGamePieces[CalcIndex(xBlock, yBlock, zBlock)];
    }

    GamePiece* GameBoard::AllocGamePiece()
    {
        assert(mGamePieceFreeList != nullptr);
        
        GamePiece* result = mGamePieceFreeList;
        mGamePieceFreeList = result->mNext;
        result->mNext = nullptr;
        return result;
    }

    const GamePiece* const* GameBoard::GetGamePieces(size_t* outNumGamePieces) const
    {
        if (outNumGamePieces != nullptr)
        {
            *outNumGamePieces = ArraySize(mGamePieces);
        }
        return mGamePieces;
    }

    void GameBoard::FreeGamePiece(GamePiece* gamePiece)
    {
        gamePiece->mNext = mGamePieceFreeList;
        mGamePieceFreeList = gamePiece;
    }

    void GameBoard::PlaceGamePiece(int xBlock, int yBlock, int zBlock, const DirectX::XMVECTOR& color, int remainingTicks, GamePieceType gamePieceType)
    {
        size_t index = CalcIndex(xBlock, yBlock, zBlock);
        assert(mGamePieces[index] == nullptr);
        
        GamePiece* gamePiece = AllocGamePiece();
        gamePiece->mRemainingTicks = remainingTicks;
        gamePiece->mGamePieceType = gamePieceType;
        gamePiece->mColor = color;
        gamePiece->mPosition = GetPosition(xBlock, yBlock, zBlock);
        mGamePieces[index] = gamePiece;
    }

    void GameBoard:: RemoveGamePiece(int xBlock, int yBlock, int zBlock)
    {
        size_t index = CalcIndex(xBlock, yBlock, zBlock);
        GamePiece* gamePiece = mGamePieces[index];
        assert(gamePiece != nullptr);

        FreeGamePiece(gamePiece);
        mGamePieces[index] = nullptr;
    }

} // namespace Snake
