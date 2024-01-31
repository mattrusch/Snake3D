// Application.h

#pragma once

#include "Window.h"
#include "Camera.h"
#include "Snake3D.h"

namespace Vnm
{
    class Device;

    class PlayerState
    {
    public:
        PlayerState() = default;
        ~PlayerState() = default;

        // TODO: Perform better initialization and get rid of sentinel
        int mCurBlockCoord[3] = { ~0, ~0, ~0 };
        int mBodyLength = 1;
    };

    class Application
    {
    public:
        Application() = default;
        ~Application() = default;

        void Startup(HINSTANCE instance, int cmdShow);
        void Mainloop();
        void Shutdown();

        void OnKeyUp(UINT8 key);
        void OnKeyDown(UINT8 key);

        bool GameIsActive() const { return mCurCamera == &mGameCamera; }

    private:
        void ToggleGameState();

        Snake::GameBoard mGameBoard;

        Window      mWindow;
        PlayerState mPlayerState;
        Camera      mSnake;
        Camera      mFreeCamera;
        Camera      mGameCamera;
        Camera*     mCurCamera = &mFreeCamera;
        uint32_t    mMoveState = 0;
    };
}
