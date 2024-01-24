#pragma once

class Camera
{
public:
    void InitCamera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 target, float frameAspectRatio);

    void UpdateCamera(DirectX::XMFLOAT2 lookRotation, DirectX::XMFLOAT3 movement);

    DirectX::XMFLOAT4X4 projMat; // this will store our projection matrix
    DirectX::XMFLOAT4X4 viewMat; // this will store our view matrix
    DirectX::XMMATRIX rotMat;

    DirectX::XMFLOAT4 position; // this is our cameras position vector
    DirectX::XMFLOAT4 target; // a vector describing the point in space our camera is looking at
    DirectX::XMFLOAT4 up; // the worlds up vector

    // Vectors defining up right and front directions local to the camera.
    DirectX::XMVECTOR localUp;
    DirectX::XMVECTOR localRight;
    DirectX::XMVECTOR localFront;
    DirectX::XMVECTOR globalUp;
    DirectX::XMVECTOR globalRight;
    DirectX::XMVECTOR globalFront;
    DirectX::XMVECTOR camTarget;
    DirectX::XMVECTOR camPosition;
};

