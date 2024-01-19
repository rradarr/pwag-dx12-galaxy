#pragma once

using Microsoft::WRL::ComPtr;

class Material
{
public:
    Material() = default;
    Material(const std::string vertexShader, const std::string pixelShader);
    void SetShaders(const std::string vertexShaderFileName, const std::string pixelShaderFileName);
    // Final step of material creation, actually compiles shaders, allocates DX resources etc.
    void CreateMaterial();

    ComPtr<ID3D12PipelineState> GetPSO() { return pipelineState; };
    ComPtr<ID3D12RootSignature> GetRootSignature() { return rootSignature; };

protected:
    ComPtr<ID3DBlob> LoadAndCompileShader(const std::string fileName, const std::string compilationTarget);
    void CreateRootSignature();
    void CreatePSO(ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader);

    // Inheriting classes can override the following methods to specialize.
    virtual std::vector<D3D12_ROOT_PARAMETER> CreateRootParameters();
    virtual D3D12_STATIC_SAMPLER_DESC CreateSampler();
    virtual D3D12_ROOT_SIGNATURE_FLAGS CreateRootSignatureFlags();
    virtual void CustomizePipelineStateObjectDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

    ComPtr<ID3D12RootSignature> rootSignature; // ? Could have a global root desc
    ComPtr<ID3D12PipelineState> pipelineState;

private:
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorTableVertexRanges;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    std::string vertexShaderFileName;
    std::string pixelShaderFileName;
};

