#pragma once

using Microsoft::WRL::ComPtr;

class Texture
{
public:
    Texture() = default;
    Texture(const std::string fileName);
    bool CreateFromFile(const std::string fileName);
    UINT GetOffsetInHeap() { return viewOffsetInHeap; }

    // Creates a view/descriptor of the texture in the heap provided.
    void CreateTextureView();
private:
    ComPtr<ID3D12Resource> textureBuffer;
    D3D12_RESOURCE_DESC textureDesc;
    UINT viewOffsetInHeap;
};
