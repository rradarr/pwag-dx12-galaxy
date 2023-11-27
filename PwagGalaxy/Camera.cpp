#include "stdafx.h"
#include "Camera.h"


void Camera::InitCamera(DirectX::XMFLOAT4 cameraPosition, DirectX::XMFLOAT4 cameraTarget, float frameAspectRatio)
{
    // build projection and view matrix
    DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), frameAspectRatio, 0.1f, 1000.0f);
    XMStoreFloat4x4(&projMat, tmpMat);

    // set starting camera state
    position = cameraPosition;
    target = cameraTarget;
    up = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

    // build view matrix
    DirectX::XMVECTOR cPos = DirectX::XMLoadFloat4(&position);
    DirectX::XMVECTOR cTarg = DirectX::XMLoadFloat4(&target);
    DirectX::XMVECTOR cUp = DirectX::XMLoadFloat4(&up);
    tmpMat = DirectX::XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&viewMat, tmpMat);
}
