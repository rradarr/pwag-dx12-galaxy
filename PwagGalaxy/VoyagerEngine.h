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
    static const UINT frameBufferCount = 3;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    // Pipeline objects
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[frameBufferCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator[frameBufferCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList; // As many as therads, so one. It can be reset immidiately after submitting.
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT frameBufferIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence[frameBufferCount];
    UINT64 m_fenceValue[frameBufferCount];

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};

