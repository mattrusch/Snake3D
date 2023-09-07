// Window.cpp

#include "Window.h"
#include "Application.h"
#include <cassert>

LRESULT CALLBACK DefaultWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    Vnm::Window* pWindow = reinterpret_cast<Vnm::Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)LPCREATESTRUCT(lparam)->lpCreateParams);
        break;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }

        if (pWindow != nullptr)
        {
            pWindow->OnKeyDown(static_cast<UINT8>(wparam));
        }

        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        break;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}

namespace Vnm
{
    void Window::Create(HINSTANCE instance, int cmdShow, const WindowDesc& desc)
    {
        assert(desc.mParentApplication != nullptr);
        mApplication = desc.mParentApplication;

        WNDCLASSEX wndClass = {0};
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wndClass.hInstance = instance;
        wndClass.lpfnWndProc = desc.mWndProc;
        wndClass.lpszClassName = desc.mpTitle;
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

        RegisterClassEx(&wndClass);

        mHandle = CreateWindow(
            desc.mpTitle, 
            desc.mpTitle, 
            WS_OVERLAPPEDWINDOW, 
            desc.mTop, 
            desc.mLeft, 
            desc.mWidth,
            desc.mHeight, 
            NULL, 
            NULL, 
            instance,
            (void*)this);

        DWORD error = GetLastError();

        ShowWindow(mHandle, cmdShow);
    }

    void Window::OnKeyDown(UINT8 key)
    {
        // Forward to application
        if (mApplication != nullptr)
        {
            mApplication->OnKeyDown(key);
        }
    }

    void Window::Destroy()
    {
        DestroyWindow(mHandle);
    }
}
