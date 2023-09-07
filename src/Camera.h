// Camera.h

#pragma once

#include <DirectXMath.h>

namespace Vnm
{

    class Camera
    {
    public:
        Camera()
            : mPosition(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f))
            , mForward(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f))
            , mUp(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f))
            , mRight(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f))
        {}
        Camera(
            DirectX::XMVECTOR position, 
            DirectX::XMVECTOR forward, 
            DirectX::XMVECTOR up, 
            DirectX::XMVECTOR right)
            : mPosition(position)
            , mForward(forward)
            , mUp(up)
            , mRight(right)
        {}
        ~Camera() = default;

        DirectX::XMMATRIX CalcLookAt() const;
        void Pitch(float radians);
        void Yaw(float radians);
        void MoveForward(float delta);
        void SetPosition(const DirectX::XMVECTOR& position) { mPosition = position; }

    private:
        DirectX::XMVECTOR mPosition;
        DirectX::XMVECTOR mForward;
        DirectX::XMVECTOR mUp;
        DirectX::XMVECTOR mRight;
    };

} // namespace vnm
