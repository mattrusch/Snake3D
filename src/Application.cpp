// Application.cpp

#include "Application.h"
#include "D3d12Context.h"

namespace Vnm
{
    void Application::Startup(HINSTANCE instance, int cmdShow)
    {
        // Create main window and device
        Window::WindowDesc winDesc;
        winDesc.mWidth = 1024;
        winDesc.mHeight = 1024;
        winDesc.mParentApplication = this;
        mWindow.Create(instance, cmdShow, winDesc);

        Init(mWindow.GetHandle());
        mCamera.SetPosition(DirectX::XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f));
    }

    void Application::Mainloop()
    {
        static uint32_t lastTime = GetTickCount();
        uint32_t elapsedTime = GetTickCount() - lastTime;
        lastTime = GetTickCount();
        float elapsedSeconds = static_cast<float>(elapsedTime) * 0.001f;

        Update( mCamera.CalcLookAt(), elapsedSeconds );
        Render();
    }

    void Application::Shutdown()
    {
        mWindow.Destroy();
    }

    void Application::OnKeyDown(UINT8 key)
    {
        const float rotationScale = 0.01f;
        const float forwardScale = 0.1f;

        switch (key)
        {
        case VK_SPACE:
            mCamera.MoveForward(forwardScale);
            break;
        case VK_SHIFT:
            mCamera.MoveForward(-forwardScale);
            break;
        case VK_LEFT:
            mCamera.Yaw( -DirectX::XM_PI * rotationScale );
            break;
        case VK_RIGHT:
            mCamera.Yaw( DirectX::XM_PI * rotationScale );
            break;
        case VK_UP:
            mCamera.Pitch( DirectX::XM_PI * rotationScale );
            break;
        case VK_DOWN:
            mCamera.Pitch( -DirectX::XM_PI * rotationScale );
            break;
        default: break;
        }
    }
}
