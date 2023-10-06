// Window.h

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

LRESULT CALLBACK DefaultWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace Vnm
{
    class Application;

    class Window
    {
    public:
        Window() = default;
        ~Window() = default;

        class WindowDesc
        {
        public:
            int          mTop = 100;
            int          mLeft = 100;
            int          mWidth = 1024;
            int          mHeight = 1024;
            LPCWSTR      mpTitle = L"Vnm Window";
            WNDPROC      mWndProc = DefaultWndProc;
            Application* mParentApplication = nullptr;
        };

        void Create(HINSTANCE instance, int cmdShow, const WindowDesc& desc);
        void Destroy();
        void OnKeyDown(UINT8 key);
        void OnKeyUp(UINT8 key);

        HWND GetHandle() const { return mHandle; }
    private:
        Application* mApplication;  // For messaging
        HWND         mHandle;
    };
}
