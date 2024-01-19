#pragma once

#include <string>

using Microsoft::WRL::ComPtr;

class Texture
{
public:
    Texture() = default;
    Texture(const std::string fileName);
    bool CreateFromFile(const std::string fileName);

    // Creates a view/descriptor of the texture in the heap provided.
    void CreateTextureView(CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle);
private:
    ComPtr<ID3D12Resource> textureBuffer;
    D3D12_RESOURCE_DESC textureDesc;
};
