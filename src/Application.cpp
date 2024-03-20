// Application.cpp

#include "Application.h"
#include "D3d12Context.h"
#include <climits>
#include <random>

namespace Vnm
{
    constexpr uint32_t MoveForwardBit = 1 << 0;
    constexpr uint32_t MoveBackBit    = 1 << 1;
    constexpr uint32_t TurnLeftBit    = 1 << 2;
    constexpr uint32_t TurnRightBit   = 1 << 3;
    constexpr uint32_t TiltUpBit      = 1 << 4;
    constexpr uint32_t TiltDownBit    = 1 << 5;
    const DirectX::XMVECTOR GameCameraOffset = DirectX::XMVectorSet(5.0f, 0.0f, 0.0f, 0.0f);

    static void SetupWalls(Snake::GameBoard& gameBoard)
    {
        // Place pieces around the borders
        for (int i = 0; i < Snake::NumPiecesX; i++)
        {
            for (int j = 0; j < Snake::NumPiecesY; j++)
            {
                for (int k = 0; k < Snake::NumPiecesZ; k++)
                {
                    if ((i == 0) || (i == Snake::NumPiecesX - 1) ||
                        (j == 0) || (j == Snake::NumPiecesY - 1) ||
                        (k == 0) || (k == Snake::NumPiecesZ - 1))
                    {
                        // TODO: Clean this up
                        DirectX::XMVECTOR color;
                        if (i == 0)
                        {
                            color = DirectX::XMVectorSet(1.0f, 0.4f, 0.4f, 1.0f);
                        }
                        else if (i == Snake::NumPiecesX - 1)
                        {
                            color = DirectX::XMVectorSet(1.0f, 0.4f, 1.0f, 1.0f);
                        }
                        else if (j == 0)
                        {
                            color = DirectX::XMVectorSet(0.4f, 0.4f, 1.0f, 1.0f);
                        }
                        else if (j == Snake::NumPiecesY - 1)
                        {
                            color = DirectX::XMVectorSet(0.4f, 1.0f, 1.0f, 1.0f);
                        }
                        else if (k == 0)
                        {
                            color = DirectX::XMVectorSet(0.4f, 1.0f, 0.4f, 1.0f);
                        }
                        else if (k == Snake::NumPiecesZ - 1)
                        {
                            color = DirectX::XMVectorSet(1.0f, 1.0f, 0.4f, 1.0f);
                        }

                        gameBoard.PlaceGamePiece(i, j, k, color, INT_MAX, Snake::GamePieceType::Wall);
                    }
                }
            }
        }
    }

    static void PlacePowerUp(Snake::GameBoard& gameBoard)
    {
        const DirectX::XMVECTOR color = DirectX::XMVectorSet(0.7f, 0.8f, 1.0f, 1.0f);

        // TODO: Give random generation a home
        static std::random_device randomDevice;
        static std::mt19937 randomGenerator(randomDevice());
        using DistributionType = std::uniform_int_distribution <std::mt19937::result_type>;
        static DistributionType distributionX(0, Snake::NumPiecesX - 1);
        static DistributionType distributionY(0, Snake::NumPiecesY - 1);
        static DistributionType distributionZ(0, Snake::NumPiecesZ - 1);

        int x = distributionX(randomGenerator);
        int y = distributionY(randomGenerator);
        int z = distributionZ(randomGenerator);

        // TODO: Will possibly endless loop at the very end of the game, check for win condition
        while (gameBoard.GetGamePiece(x, y, z) != nullptr)
        {
            // If random place is already occupied, find another
            x = distributionX(randomGenerator);
            y = distributionY(randomGenerator);
            z = distributionZ(randomGenerator);
        }

        gameBoard.PlaceGamePiece(x, y, z, color, INT_MAX, Snake::GamePieceType::PowerUp);
    }

    void Application::Startup(HINSTANCE instance, int cmdShow)
    {
        // Create main window and device
        Window::WindowDesc winDesc;
        winDesc.mWidth = 1024;
        winDesc.mHeight = 1024;
        winDesc.mParentApplication = this;
        mWindow.Create(instance, cmdShow, winDesc);

        Init(mWindow.GetHandle());
        mSnake.SetPosition(DirectX::XMVectorSet(5.0f, 5.0f, 5.0f, 0.0f));
        mFreeCamera.SetPosition(DirectX::XMVectorSet(5.0f, 5.0f, 5.0f, 0.0f));

        mGameBoard.Init();
        SetupWalls(mGameBoard);
        PlacePowerUp(mGameBoard);
    }

    void Application::Reset()
    {
        mSnake.SetPosition(DirectX::XMVectorSet(5.0f, 5.0f, 5.0f, 0.0f));
        mSnake.ResetBasis();
        mGameBoard.Reset();
        SetupWalls(mGameBoard);
        PlacePowerUp(mGameBoard);
        mPlayerState.mBodyLength = 1;
    }

    static void HandleMovement(uint32_t key, Camera& camera)
    {
        const float rotationScale = 0.01f;
        const float forwardScale = 0.1f;

        if (key & MoveForwardBit)
        {
            camera.MoveForward(forwardScale);
        }
        if (key & MoveBackBit)
        {
            camera.MoveForward(-forwardScale);
        }
        if (key & TurnLeftBit)
        {
            camera.Yaw(-DirectX::XM_PI * rotationScale);
        }
        if (key & TurnRightBit)
        {
            camera.Yaw(DirectX::XM_PI * rotationScale);
        }
        if (key & TiltDownBit)
        {
            camera.Pitch(DirectX::XM_PI * rotationScale);
        }
        if (key & TiltUpBit)
        {
            camera.Pitch(-DirectX::XM_PI * rotationScale);
        }
    }

    static void HandleMovementGame(uint32_t key, Camera& camera)
    {
        const float rotationScale = 0.5f;
        const float forwardScale = 0.025f;

        camera.MoveForward(forwardScale);

        if (key & TurnLeftBit)
        {
            camera.Yaw(-DirectX::XM_PI * rotationScale);
        }
        if (key & TurnRightBit)
        {
            camera.Yaw(DirectX::XM_PI * rotationScale);
        }
        if (key & TiltDownBit)
        {
            camera.Pitch(DirectX::XM_PI * rotationScale);
        }
        if (key & TiltUpBit)
        {
            camera.Pitch(-DirectX::XM_PI * rotationScale);
        }
    }

    void Application::Mainloop()
    {
        static uint32_t lastTime = GetTickCount();
        uint32_t elapsedTime = GetTickCount() - lastTime;
        lastTime = GetTickCount();

        const float timeScale = 0.001f;
        float elapsedSeconds = static_cast<float>(elapsedTime) * timeScale;

        if (GameIsActive())
        {
            HandleMovementGame(mMoveState, mSnake);
            mMoveState = 0;

            int xBlockCoord;
            int yBlockCoord;
            int zBlockCoord;
            mGameBoard.GetBlockCoords(mSnake.GetPosition(), xBlockCoord, yBlockCoord, zBlockCoord);

            // Check if snake head has moved into a new block
            if (xBlockCoord != mPlayerState.mCurBlockCoord[0] ||
                yBlockCoord != mPlayerState.mCurBlockCoord[1] ||
                zBlockCoord != mPlayerState.mCurBlockCoord[2])
            {
                // Test for intersection
                const Snake::GamePiece* gamePiece = mGameBoard.GetGamePiece(xBlockCoord, yBlockCoord, zBlockCoord);
                if (gamePiece == nullptr)
                {
                    mGameBoard.PlaceGamePiece(xBlockCoord, yBlockCoord, zBlockCoord, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), mPlayerState.mBodyLength, Snake::GamePieceType::SnakeBody);
                }
                else
                {
                    // Hitting wall or snake piece ends game
                    if (gamePiece->mGamePieceType == Snake::GamePieceType::SnakeBody ||
                        gamePiece->mGamePieceType == Snake::GamePieceType::Wall)
                    {
                        Reset();
                        ToggleGameState();
                    }

                    // Powerup increases length
                    if (gamePiece->mGamePieceType == Snake::GamePieceType::PowerUp)
                    {
                        // Replace power-up with snake body piece
                        mGameBoard.RemoveGamePiece(xBlockCoord, yBlockCoord, zBlockCoord);

                        // Place a new power-up
                        PlacePowerUp(mGameBoard);

                        // Increase body length
                        mGameBoard.PlaceGamePiece(xBlockCoord, yBlockCoord, zBlockCoord, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), ++mPlayerState.mBodyLength, Snake::GamePieceType::SnakeBody);

                        // TODO: Test win condition
                    }
                }

                mPlayerState.mCurBlockCoord[0] = xBlockCoord;
                mPlayerState.mCurBlockCoord[1] = yBlockCoord;
                mPlayerState.mCurBlockCoord[2] = zBlockCoord;

                // Update pieces on board (excluding walls)
                for (int i = 1; i < Snake::NumPiecesX - 1; i++)
                {
                    for (int j = 1; j < Snake::NumPiecesY - 1; j++)
                    {
                        for (int k = 1; k < Snake::NumPiecesZ - 1; k++)
                        {
                            Snake::GamePiece* gamePiece = mGameBoard.GetGamePiece(i, j, k);
                            if (gamePiece == nullptr)
                            {
                                continue;
                            }
                            else if (gamePiece->mGamePieceType == Snake::GamePieceType::SnakeBody)
                            {
                                if (gamePiece->mRemainingTicks-- == 0)
                                {
                                    mGameBoard.RemoveGamePiece(i, j, k);
                                }
                            }
                        }
                    }
                }
            }

            // Look at snake head from behind and above
            const float positionOffsetScale = 3.0f;
            static DirectX::XMVECTOR prevPositionOffset = DirectX::XMVectorSet(positionOffsetScale, 0.0f, 0.0f, 0.0f);

            // Cheap smooth follow, slerp would probably be better
            DirectX::XMVECTOR positionOffset = DirectX::XMVectorScale(DirectX::XMVectorSubtract(mSnake.GetUp(), mSnake.GetForward()), positionOffsetScale);
            DirectX::XMVECTOR length = DirectX::XMVector3Length(positionOffset);

            // Forcing consistent length here might be overkill
            const float lerpFactor = 0.1f;
            positionOffset = DirectX::XMVectorLerp(prevPositionOffset, positionOffset, lerpFactor);
            positionOffset = DirectX::XMVector3Normalize(positionOffset);
            positionOffset = DirectX::XMVectorMultiply(positionOffset, length);
            prevPositionOffset = positionOffset;
            
            // Blend right vector as well
            static DirectX::XMVECTOR prevRight = mSnake.GetRight();
            DirectX::XMVECTOR right = mSnake.GetRight();
            right = DirectX::XMVectorLerp(prevRight, right, lerpFactor);
            prevRight = right;

            mGameCamera.SetPosition(DirectX::XMVectorAdd(mSnake.GetPosition(), positionOffset));
            mGameCamera.SetLookAtRecalcBasis(mSnake.GetPosition(), right);
        }
        else
        {
            HandleMovement(mMoveState, *mCurCamera);
        }

        Update( mCurCamera->CalcLookAt(), elapsedSeconds );
        size_t numGamePieces;
        const Snake::GamePiece* const* gamePieces = mGameBoard.GetGamePieces( &numGamePieces );
        Render( gamePieces, numGamePieces, mCurCamera->CalcLookAt(), elapsedSeconds );
    }

    void Application::Shutdown()
    {
        mWindow.Destroy();
    }

    void Application::OnKeyUp(UINT8 key)
    {
        switch (key)
        {
        case VK_SPACE:
            mMoveState &= ~MoveForwardBit;
            break;
        case VK_SHIFT:
            mMoveState &= ~MoveBackBit;
            break;
        case VK_LEFT:
        case 'A':
            mMoveState &= ~TurnLeftBit;
            break;
        case VK_RIGHT:
        case 'D':
            mMoveState &= ~TurnRightBit;
            break;
        case VK_UP:
        case 'W':
            mMoveState &= ~TiltDownBit;
            break;
        case VK_DOWN:
        case 'S':
            mMoveState &= ~TiltUpBit;
            break;
        default: break;
        }
    }

    void Application::ToggleGameState()
    {
        // Swap active camera
        mCurCamera = mCurCamera == &mFreeCamera ? &mGameCamera : &mFreeCamera;
    }

    void Application::OnKeyDown(UINT8 key)
    {
        switch (key)
        {
        case VK_TAB:
            ToggleGameState();
            break;
        case VK_SPACE:
            mMoveState |= MoveForwardBit;
            break;
        case VK_SHIFT:
            mMoveState |= MoveBackBit;
            break;
        case VK_LEFT:
        case 'A':
            mMoveState |= TurnLeftBit;
            break;
        case VK_RIGHT:
        case 'D':
            mMoveState |= TurnRightBit;
            break;
        case VK_UP:
        case 'W':
            mMoveState |= TiltDownBit;
            break;
        case VK_DOWN:
        case 'S':
            mMoveState |= TiltUpBit;
            break;
        default: break;
        }
    }

}
