#pragma once

#include "stdafx.h"

class Camera
{
public:
    void InitCamera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 target, float frameAspectRatio);

    DirectX::XMFLOAT4X4 projMat; // this will store our projection matrix
    DirectX::XMFLOAT4X4 viewMat; // this will store our view matrix

    DirectX::XMFLOAT4 position; // this is our cameras position vector
    DirectX::XMFLOAT4 target; // a vector describing the point in space our camera is looking at
    DirectX::XMFLOAT4 up; // the worlds up vector
};

