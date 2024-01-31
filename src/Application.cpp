// Application.cpp

#include "Application.h"
#include "D3d12Context.h"
#include <climits>

namespace Vnm
{
    constexpr uint32_t MoveForwardBit = 1 << 0;
    constexpr uint32_t MoveBackBit    = 1 << 1;
    constexpr uint32_t TurnLeftBit    = 1 << 2;
    constexpr uint32_t TurnRightBit   = 1 << 3;
    constexpr uint32_t TiltUpBit      = 1 << 4;
    constexpr uint32_t TiltDownBit    = 1 << 5;
    const DirectX::XMVECTOR GameCameraOffset = DirectX::XMVectorSet(5.0f, 0.0f, 0.0f, 0.0f);

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

                        mGameBoard.PlaceGamePiece(i, j, k, color, INT_MAX, Snake::GamePieceType::Wall);
                    }
                }
            }
        }
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
        float elapsedSeconds = static_cast<float>(elapsedTime) * 0.001f;

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
                        // TODO: Stand in until reset is implemented
                        ToggleGameState();
                    }

                    // Powerup increases length
                    // TODO
                }

                mPlayerState.mCurBlockCoord[0] = xBlockCoord;
                mPlayerState.mCurBlockCoord[1] = yBlockCoord;
                mPlayerState.mCurBlockCoord[2] = zBlockCoord;
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
