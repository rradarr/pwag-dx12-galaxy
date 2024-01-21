#pragma once

class WireframeMaterial : public Material
{
public:
    WireframeMaterial() = default;
    WireframeMaterial(const std::string vertexShaderFileName, const std::string pixelShaderFileName) {
        Material(vertexShaderFileName, pixelShaderFileName);
    };

private:
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    // Inheriting classes can override the following methods to specialize.
    virtual std::vector<D3D12_ROOT_PARAMETER> CreateRootParameters();
    virtual void CustomizePipelineStateObjectDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
};
