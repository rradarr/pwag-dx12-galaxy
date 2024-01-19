#pragma once
#include "Material.h"
class DefaultTexturedMaterial : public Material
{
public:
    DefaultTexturedMaterial() = default;
    DefaultTexturedMaterial(const std::string vertexShaderFileName, const std::string pixelShaderFileName);

private:
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorTableVertexRanges;
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorTablePixelRanges;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    // Inheriting classes can override the following methods to specialize.
    virtual std::vector<D3D12_ROOT_PARAMETER> CreateRootParameters();
    virtual D3D12_ROOT_SIGNATURE_FLAGS CreateRootSignatureFlags();
};
