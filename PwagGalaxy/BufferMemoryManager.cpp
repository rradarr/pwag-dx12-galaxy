#include "stdafx.h"
#include "BufferMemoryManager.h"

#include "dx_includes/DXSampleHelper.h"
#include "DXContext.h"

bool BufferMemoryManager::initialized = false;
ComPtr<ID3D12CommandAllocator> BufferMemoryManager::commandAllocator;
ComPtr<ID3D12GraphicsCommandList> BufferMemoryManager::commandList;
ComPtr<ID3D12CommandQueue> BufferMemoryManager::commandQueue;
ComPtr<ID3D12Fence> BufferMemoryManager::fence;
HANDLE BufferMemoryManager::fenceEvent;
UINT64 BufferMemoryManager::fenceValue;

BufferMemoryManager::BufferMemoryManager()
{
    if (!initialized)
    {
        Initialize();
    }
}

BufferMemoryManager::~BufferMemoryManager()
{
    FlushAndWait();
    ResetCommandListAndAllocator();
}

void BufferMemoryManager::AllocateBuffer(ComPtr<ID3D12Resource>& bufferResource, UINT bufferSize, D3D12_RESOURCE_STATES bufferState, D3D12_HEAP_TYPE heapType)
{
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        bufferState,
        nullptr,
        IID_PPV_ARGS(&bufferResource)));
}

void BufferMemoryManager::AllocateBuffer(ComPtr<ID3D12Resource>& bufferResource, CD3DX12_RESOURCE_DESC* resourceDescriptor, D3D12_RESOURCE_STATES bufferState, D3D12_HEAP_TYPE heapType, D3D12_CLEAR_VALUE* optimizedClearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        resourceDescriptor,
        bufferState,
        optimizedClearValue,
        IID_PPV_ARGS(&bufferResource)));
}

void BufferMemoryManager::FillBuffer(ComPtr<ID3D12Resource>& bufferResource, D3D12_SUBRESOURCE_DATA data, ComPtr<ID3D12Resource>& uploadBufferResource, D3D12_RESOURCE_STATES finalBufferState, bool forceFlushAndWait)
{
    // This starts a command list and records a copy command. (The list is still recording and will need to be closed and executed).

    usedResources.push_back(uploadBufferResource);

    UpdateSubresources(commandList.Get(), bufferResource.Get(), usedResources.back().Get(), 0, 0, 1, &data);
    // Set a resource barrier to transition the buffer from OCPY_DEST to a VERTEX_AND_CONSTANT_BUFFER. (this is also a command).
    CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(bufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalBufferState);
    commandList->ResourceBarrier(1, &transitionBarrier);

    cmdNeedsFlushing = true;
    cmdNeedsResetting = true;

    if (forceFlushAndWait)
    {
        FlushAndWait();
        ResetCommandListAndAllocator();
    }
}

void BufferMemoryManager::Initialize()
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    // Create the command list.
    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // Create sync objects
    ThrowIfFailed(DXContext::getDevice().Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceValue = 0;

    // Create an event handle to use for synchronization.
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    initialized = true;
}

void BufferMemoryManager::FlushAndWait()
{
    if (cmdNeedsFlushing)
    {
        // Execute the command list.
        commandList->Close();
        ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);
        fenceValue++;
        commandQueue->Signal(fence.Get(), fenceValue);
        WaitForFence();

        usedResources.clear();

        cmdNeedsFlushing = false;
    }
}

void BufferMemoryManager::WaitForFence()
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        // The m_fenceEvent will trigger when the fence for the current frame buffer reaches the specified value.
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void BufferMemoryManager::ResetCommandListAndAllocator()
{
    if (cmdNeedsResetting)
    {
        // Reset the command allocator and list, so they can be used again.
        ThrowIfFailed(commandAllocator->Reset());
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

        cmdNeedsResetting = false;
    }
}
