// Application.h

#pragma once

#include "Window.h"
#include "Camera.h"
#include "Snake3D.h"

namespace Vnm
{
    class Device;
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

        Window    mWindow;
        Camera    mSnake;
        Camera    mFreeCamera;
        Camera    mGameCamera;
        Camera*   mCurCamera = &mFreeCamera;
        uint32_t  mMoveState = 0;
    };
}
