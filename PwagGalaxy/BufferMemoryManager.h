#pragma once

using Microsoft::WRL::ComPtr;

class BufferMemoryManager
{
public:
    BufferMemoryManager();
    ~BufferMemoryManager();

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
        D3D12_CLEAR_VALUE* optimizedClearValue = nullptr);
    // Copy data to Default buffer with the use of an Upload buffer. The bool ForceFlushAndWaid will submit the command list and wait for GPU to finish.
    void FillBuffer(
        ComPtr<ID3D12Resource>& bufferResource,
        D3D12_SUBRESOURCE_DATA data,
        ComPtr<ID3D12Resource>& uploadBufferResource,
        D3D12_RESOURCE_STATES finalBufferState,
        bool forceFlushAndWait = false);
private:
    // Initialize static members of the class.
    static void Initialize();
    void FlushAndWait();
    void WaitForFence();
    void ResetCommandListAndAllocator();

    static bool initialized;
    bool cmdNeedsFlushing = false;
    bool cmdNeedsResetting = false;
    // Resources used in a cmd list must not be deleted before closing the list,
    // so we need to store them here until list is closed in the destructor.
    std::vector<ComPtr<ID3D12Resource>> usedResources;

    static ComPtr<ID3D12CommandAllocator> commandAllocator;
    static ComPtr<ID3D12GraphicsCommandList> commandList;
    static ComPtr<ID3D12CommandQueue> commandQueue;

    // synchronization
    static ComPtr<ID3D12Fence> fence;
    static HANDLE fenceEvent;
    static UINT64 fenceValue;
};

