// Application.h

#pragma once

#include "Window.h"
#include "Camera.h"

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

        void OnKeyDown(UINT8 key);
    private:
        Window mWindow;
        Camera mCamera;
    };
}
