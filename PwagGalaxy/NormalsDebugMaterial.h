#pragma once
#include "Material.h"
class NormalsDebugMaterial : public Material
{
public:
    NormalsDebugMaterial() = default;
    NormalsDebugMaterial(const std::string vertexShaderFileName, const std::string pixelShaderFileName) {
        Material(vertexShaderFileName, pixelShaderFileName);
    };

private:
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    // Inheriting classes can override the following methods to specialize.
    virtual std::vector<D3D12_ROOT_PARAMETER> CreateRootParameters();
};

