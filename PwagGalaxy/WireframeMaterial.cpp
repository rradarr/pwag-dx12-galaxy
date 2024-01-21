#include "stdafx.h"
#include "Material.h"
#include "WireframeMaterial.h"

std::vector<D3D12_ROOT_PARAMETER> WireframeMaterial::CreateRootParameters()
{
    // Create the root descriptor (for wvp matrices)
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.ShaderRegister = 1; // b1 in shader
    rootCBVDescriptor.RegisterSpace = 0;

    // create a root parameter and fill it out
    rootParameters.resize(1);
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor = rootCBVDescriptor;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    return rootParameters;
}

void WireframeMaterial::CustomizePipelineStateObjectDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
}
