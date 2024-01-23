#pragma once
#include "Material.h"

class LitMaterial : public Material
{
public:
    LitMaterial() = default;
    LitMaterial(const std::string vertexShaderFileName, const std::string pixelShaderFileName);

private:
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorTablePixelRanges;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    // Inheriting classes can override the following methods to specialize.
    virtual std::vector<D3D12_ROOT_PARAMETER> CreateRootParameters();
    virtual D3D12_ROOT_SIGNATURE_FLAGS CreateRootSignatureFlags();
};

