#include "stdafx.h"
#include "Material.h"

#include "dx_includes/DXSampleHelper.h"
#include "DXContext.h"
#include "EngineHelpers.h"



Material::Material(const std::string vertexShaderFileName, const std::string pixelShaderFileName)
{
    SetShaders(vertexShaderFileName, pixelShaderFileName);
}

void Material::SetShaders(const std::string vertexShaderFileName, const std::string pixelShaderFileName)
{
    this->vertexShaderFileName = vertexShaderFileName;
    this->pixelShaderFileName = pixelShaderFileName;
}

void Material::CreateMaterial()
{
    ComPtr<ID3DBlob> vertexShader = LoadAndCompileShader(vertexShaderFileName, "vs_5_1");
    ComPtr<ID3DBlob> pixelShader = LoadAndCompileShader(pixelShaderFileName, "ps_5_1");

    CreateRootSignature();
    CreatePSO(vertexShader, pixelShader);
}

ComPtr<ID3DBlob> Material::LoadAndCompileShader(const std::string fileName, const std::string compilationTarget)
{
    std::wstring tmpName(fileName.begin(), fileName.end());
    LPCWSTR wideFileName = tmpName.c_str();

    ComPtr<ID3DBlob> shader;
    ComPtr<ID3DBlob> shaderErrors;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    // When debugging, compiling at runtime provides descriptive errors. At release, precompiled shader bytecode should be loaded.
    if (FAILED(D3DCompileFromFile(
                    EngineHelpers::GetAssetFullPath(wideFileName).c_str(),
                    nullptr,
                    nullptr,
                    "main",
                    compilationTarget.c_str(),
                    compileFlags,
                    0,
                    &shader,
                    shaderErrors.GetAddressOf())))
    {
        OutputDebugStringA((char*)shaderErrors->GetBufferPointer());
        throw "Shader compilation failed!";
    }

    return shader;
}

void Material::CreateRootSignature()
{
    rootParameters = CreateRootParameters();

    D3D12_STATIC_SAMPLER_DESC sampler = CreateSampler();

    D3D12_ROOT_SIGNATURE_FLAGS flags = CreateRootSignatureFlags();

    // Root parameters (first argument) can be root constants, root descriptors or descriptor tables.
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        rootParameters.size(),
        rootParameters.data(),
        1,
        &sampler,
        flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(DXContext::getDevice().Get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Material::CreatePSO(ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader)
{
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Prepare the depth/stencil descpriptor for the PSO creation
    D3D12_DEPTH_STENCIL_DESC dtDesc = {};
    dtDesc.DepthEnable = TRUE;
    dtDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dtDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // More efficient than _LESS_EQUAL as less fragments pass the test.
    dtDesc.StencilEnable = FALSE; // disable stencil test
    dtDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    dtDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    dtDesc.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
    dtDesc.BackFace = defaultStencilOp;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // TODO: This changes between solid and wireframe.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = dtDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    //psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;

    CustomizePipelineStateObjectDescription(psoDesc);

    ThrowIfFailed(DXContext::getDevice().Get()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
}

std::vector<D3D12_ROOT_PARAMETER> Material::CreateRootParameters()
{
    // create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    descriptorTableVertexRanges.resize(1); // only one range right now
    descriptorTableVertexRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
    descriptorTableVertexRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
    descriptorTableVertexRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
    descriptorTableVertexRanges[0].RegisterSpace = 0; // space 0. can usually be zero
    descriptorTableVertexRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

    // create a descriptor table for the vertex stage
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableVertex;
    descriptorTableVertex.NumDescriptorRanges = descriptorTableVertexRanges.size();
    descriptorTableVertex.pDescriptorRanges = descriptorTableVertexRanges.data(); // the pointer to the beginning of our ranges array

    // Create the root descriptor (for wvp matrices)
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.ShaderRegister = 1; // b1 in shader
    rootCBVDescriptor.RegisterSpace = 0;

    // create a root parameter and fill it out
    rootParameters.resize(2);
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[0].DescriptorTable = descriptorTableVertex; // this is our descriptor table for this root parameter
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // vertex will access cbv

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor = rootCBVDescriptor;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    return rootParameters;
}

D3D12_STATIC_SAMPLER_DESC Material::CreateSampler()
{
    // Create the static sample description.
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0; // s0 in shader
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    return sampler;
}

D3D12_ROOT_SIGNATURE_FLAGS Material::CreateRootSignatureFlags()
{
    D3D12_ROOT_SIGNATURE_FLAGS flags = (
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    return flags;
}

void Material::CustomizePipelineStateObjectDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{

}
