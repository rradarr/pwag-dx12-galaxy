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

    localUp = DirectX::XMLoadFloat4(&up);
    localFront = DirectX::XMVector4Normalize(DirectX::XMVectorSubtract(DirectX::XMLoadFloat4(&target), DirectX::XMLoadFloat4(&position)));
    localRight = DirectX::XMVector3Cross(localUp, localFront);

    globalUp = {0, 1, 0};
    globalRight = {1, 0, 0};
    globalFront = {0, 0, 1};
    camTarget = DirectX::XMLoadFloat4(&target);
    camPosition = DirectX::XMLoadFloat4(&position);
}

void Camera::UpdateCamera(DirectX::XMFLOAT2 lookRotation, DirectX::XMFLOAT3 movement)
{
    DirectX::XMMATRIX camRotationMatrix;
    float camYaw = lookRotation.x * 0.005;
    float camPitch = lookRotation.y * 0.005;
    camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);

    DirectX::XMVECTOR rotAngles = DirectX::XMVECTOR{ camPitch , camYaw, 0 };

    DirectX::XMMATRIX localXrot = DirectX::XMMatrixRotationAxis(localRight, camPitch);
    DirectX::XMMATRIX localYrot = DirectX::XMMatrixRotationAxis(localUp, camYaw);
    //camTarget = DirectX::XMVector3TransformCoord(localFront, camRotationMatrix);
    //camTarget = DirectX::XMVector3Normalize(camTarget);

    rotMat = localXrot * localYrot;

    localRight = DirectX::XMVector3TransformNormal(localRight, localXrot);
    localUp = DirectX::XMVector3TransformNormal(localUp, localXrot);
    localFront = DirectX::XMVector3TransformNormal(localFront, localXrot);

    localRight = DirectX::XMVector3TransformNormal(localRight, localYrot);
    localUp = DirectX::XMVector3TransformNormal(localUp, localYrot);
    localFront = DirectX::XMVector3TransformNormal(localFront, localYrot);

    localRight = DirectX::XMVector3Normalize(localRight);
    localUp = DirectX::XMVector3Normalize(localUp);
    localFront = DirectX::XMVector3Normalize(localFront);
    /*localRight = DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(localRight, camRotationMatrix));
    localFront = DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(localFront, camRotationMatrix));
    localUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(localFront, localRight));*/

    camTarget = localFront;

    float moveSpeedScale = 0.01;
    camPosition = DirectX::XMVectorAdd(camPosition, DirectX::XMVectorScale(localRight, movement.x * moveSpeedScale));
    camPosition = DirectX::XMVectorAdd(camPosition, DirectX::XMVectorScale(localUp, movement.y * moveSpeedScale));
    camPosition = DirectX::XMVectorAdd(camPosition, DirectX::XMVectorScale(localFront, movement.z * moveSpeedScale));

    camTarget = DirectX::XMVectorAdd(camPosition, camTarget);

    DirectX::XMStoreFloat4x4(&viewMat, DirectX::XMMatrixLookAtLH(camPosition, camTarget, localUp));
}
