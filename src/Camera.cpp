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
        //DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0, 0.0f, 0.0f), radians);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(mUp, radians);
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

    // Recalculates forward and up based on current position and lookAtPos and right arguments
    void Camera::SetLookAtRecalcBasis(const DirectX::XMVECTOR& lookAtPos, const DirectX::XMVECTOR& right)
    {
        // TODO: Assert that right is unit length?
        DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(lookAtPos, mPosition));
        DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

        mForward = forward;
        mUp = up;
        mRight = right;
    }
}
