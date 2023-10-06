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
    private:
        Snake::GameBoard mGameBoard;

        Window   mWindow;
        Camera   mCamera;
        uint32_t mMoveState = 0;
    };
}
