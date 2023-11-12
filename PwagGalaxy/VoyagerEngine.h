#pragma once

#include "stdafx.h"

#include "Engine.h"

using Microsoft::WRL::ComPtr;

class VoyagerEngine : public Engine
{
public:
    VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

    virtual void OnKeyDown(UINT8 keyCode);
    virtual void OnKeyUp(UINT8 keyCode);

private:
    static const UINT mc_frameBufferCount = 3;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    // this is the structure of our constant buffer.
    struct ConstantBuffer {
        DirectX::XMFLOAT4 colorMultiplier;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport; // Area that the view-space (rasterizer outputt, between 0,1) will be streched to (and make up the screen-space).
    CD3DX12_RECT m_scissorRect; // Area in cscreen-space that will be drawn.
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[mc_frameBufferCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator[mc_frameBufferCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList; // As many as therads, so one. It can be reset immidiately after submitting.
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature; // Defines all data that will be used by the shaders (all reasource descriptors + constants).
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState; // Our PSO. Will need more of them. Defines the state of the pipeline (duh).
    UINT m_rtvDescriptorSize;

    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_dsHeap;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView; // Contains a pointer to the vertex buffer, size of buffer and size of each element.
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    // Constant Descriptor Table resources.
    ComPtr<ID3D12DescriptorHeap> m_mainHeap[mc_frameBufferCount];
    ComPtr<ID3D12Resource> m_constantBufferUpload[mc_frameBufferCount];
    ConstantBuffer m_cbData;
    UINT8* cbColorMultiplierGPUAddress[mc_frameBufferCount]; // Pointer to the memory location we get when we map our constant buffer.

    // Synchronization objects.
    UINT m_frameBufferIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence[mc_frameBufferCount];
    UINT64 m_fenceValue[mc_frameBufferCount];

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();

    void AllocateBuffer(
        ComPtr<ID3D12Resource>& bufferResource,
        UINT bufferSize,
        D3D12_RESOURCE_STATES bufferState,
        D3D12_HEAP_TYPE heapType);

    void AllocateBuffer(
        ComPtr<ID3D12Resource>& bufferResource,
        CD3DX12_RESOURCE_DESC* resourceDescriptor,
        D3D12_RESOURCE_STATES bufferState,
        D3D12_HEAP_TYPE heapType,
        D3D12_CLEAR_VALUE optimizedClearValue);

    void FillBuffer(
        ComPtr<ID3D12Resource>&  bufferResource,
        D3D12_SUBRESOURCE_DATA data,
        ComPtr<ID3D12Resource>&  uploadBufferResource,
        D3D12_RESOURCE_STATES finalBufferState,
        bool waitForGPU = true);
};

