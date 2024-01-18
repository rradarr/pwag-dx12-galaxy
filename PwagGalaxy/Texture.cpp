#include "stdafx.h"
#include "Texture.h"

#include "DXContext.h"
#include "EngineHelpers.h"
#include "BufferMemoryManager.h"
#include "TextureLoader.h"

Texture::Texture(const std::string fileName)
{
    CreateFromFile(fileName);
}

bool Texture::CreateFromFile(const std::string fileName)
{
    std::wstring tmpName(fileName.begin(), fileName.end());
    LPCWSTR wideFileName = tmpName.c_str();

    BufferMemoryManager buffMng;
    TextureLoader textureLoader;
    int imageBytesPerRow;
    BYTE* imageData;
    int imageSize = textureLoader.LoadTextureFromFile(&imageData, textureDesc, EngineHelpers::GetAssetFullPath(wideFileName).c_str(), imageBytesPerRow);

    CD3DX12_RESOURCE_DESC desc(textureDesc);
    UINT64 textureUploadBufferSize = 0;
    DXContext::getDevice().Get()->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
    ComPtr<ID3D12Resource> textureUploadBuffer;
    buffMng.AllocateBuffer(textureBuffer, &desc, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);
    buffMng.AllocateBuffer(textureUploadBuffer, textureUploadBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &imageData[0];
    textureData.RowPitch = imageBytesPerRow;
    textureData.SlicePitch = imageBytesPerRow * textureDesc.Height;
    buffMng.FillBuffer(textureBuffer, textureData, textureUploadBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    textureBuffer->SetName(L"Texture buffer resource");

    // Create the SRV Descriptor Heap
    /*D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SRVHeap)));
    m_SRVHeap->SetName(L"SRV Heap");*/

    return true;
}

void Texture::CreateTextureView(CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    DXContext::getDevice().Get()->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, heapHandle);
}
