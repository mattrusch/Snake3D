// Camera.cpp

#include "Camera.h"

namespace Vnm
{
    // Calculates and returns LookAt matrix
    DirectX::XMMATRIX Camera::CalcLookAt() const
    {
        DirectX::XMMATRIX result = DirectX::XMMatrixLookAtLH(mPosition, DirectX::XMVectorAdd(mPosition, mForward), mUp);
        return result;
    }

    void Camera::Pitch(float radians)
    {
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(mRight, radians);
        mForward = DirectX::XMVector3Transform(mForward, rotation);
        mUp = DirectX::XMVector3Transform(mUp, rotation);
    }

    void Camera::Yaw(float radians)
    {
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0, 0.0f, 0.0f), radians);
        mForward = DirectX::XMVector3Transform(mForward, rotation);
        mRight = DirectX::XMVector3Transform(mRight, rotation);
        mUp = DirectX::XMVector3Transform(mUp, rotation);
    }

    // Moves camera in direction of forward vector
    void Camera::MoveForward(float delta)
    {
        DirectX::XMVECTOR scale = DirectX::XMVectorSet(delta, delta, delta, 0.0f);
        DirectX::XMVECTOR scaledForward = DirectX::XMVectorMultiply(mForward, scale);
        mPosition = DirectX::XMVectorAdd(mPosition, scaledForward);
    }
}
