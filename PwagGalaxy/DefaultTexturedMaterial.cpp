#include "stdafx.h"
#include "DefaultTexturedMaterial.h"

DefaultTexturedMaterial::DefaultTexturedMaterial(const std::string vertexShaderFileName, const std::string pixelShaderFileName)
{
    Material(vertexShaderFileName, pixelShaderFileName);
}

std::vector<D3D12_ROOT_PARAMETER> DefaultTexturedMaterial::CreateRootParameters()
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

    // Create the descriptor range for the texture resource (it needs to be a separate root parameter as it's used by a different shader stage!)
    descriptorTablePixelRanges.resize(1);
    descriptorTablePixelRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorTablePixelRanges[0].NumDescriptors = 1;
    descriptorTablePixelRanges[0].BaseShaderRegister = 0; // t0 in shader
    descriptorTablePixelRanges[0].RegisterSpace = 0;
    descriptorTablePixelRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // create a descriptor table for the pixel stage
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTablePixel;
    descriptorTablePixel.NumDescriptorRanges = descriptorTablePixelRanges.size();
    descriptorTablePixel.pDescriptorRanges = descriptorTablePixelRanges.data(); // the pointer to the beginning of our ranges array

    // Create the root descriptor (for wvp matrices)
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.ShaderRegister = 1; // b1 in shader
    rootCBVDescriptor.RegisterSpace = 0;

    // create a root parameter and fill it out
    rootParameters.resize(3);
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[0].DescriptorTable = descriptorTableVertex; // this is our descriptor table for this root parameter
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // vertex will access cbv

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor = rootCBVDescriptor;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[2].DescriptorTable = descriptorTablePixel; // this is our descriptor table for this root parameter
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // pixel will access srv (texture)

    return rootParameters;
}

D3D12_ROOT_SIGNATURE_FLAGS DefaultTexturedMaterial::CreateRootSignatureFlags()
{
    D3D12_ROOT_SIGNATURE_FLAGS flags = (
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    return flags;
}
