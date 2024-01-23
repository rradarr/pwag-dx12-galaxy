#pragma once

#include "Engine.h"
#include "Camera.h"
#include "Mesh.h"
#include "Noise.h"
#include "RenderingComponents.h"

using Microsoft::WRL::ComPtr;

class VoyagerEngine : public Engine
{
public:
    VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName);

    virtual void OnInit(HWND windowHandle);
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

    virtual void OnKeyDown(UINT8 keyCode);
    virtual void OnKeyUp(UINT8 keyCode);
    virtual void OnMouseMove(int mouseX, int mouseY);
    virtual void OnGotFocus();
    virtual void OnLostFocus();

private:
    static const UINT mc_frameBufferCount = 3;

    // This is the structure of the color constant buffer (used in the root desriptor table).
    struct ColorConstantBuffer {
        DirectX::XMFLOAT4 colorMultiplier;
    };
    // this is the structure of the world, view, projection constant buffer (passed as a root descriptor).
    struct wvpConstantBuffer {
        //union {
            DirectX::XMFLOAT4X4 wvpMat;
            DirectX::XMFLOAT4X4 worldMat;
            DirectX::XMFLOAT4X4 viewMat;
            DirectX::XMFLOAT4X4 projectionMat;
            // space for one more?
            //BYTE padding[256]; // Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
        //};
    };
    struct lightParamsConstantBuffer {
        union {
            DirectX::XMFLOAT3 lightPosition;
            BYTE padding[256]; // Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
        };
    };
    //int ConstantBufferPerObjectAlignedSize = (sizeof(wvpConstantBuffer) + 255) & ~255;

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport; // Area that the view-space (rasterizer outputt, between 0,1) will be streched to (and make up the screen-space).
    CD3DX12_RECT m_scissorRect; // Area in cscreen-space that will be drawn.
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[mc_frameBufferCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator[mc_frameBufferCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList; // As many as therads, so one. It can be reset immidiately after submitting.
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    UINT m_rtvDescriptorSize;

    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_dsHeap;
    // For main shader access heap the static ShaderResourceHeapManager is used.

    // App resources.
    DefaultTexturedMaterial materialTextured;
    Material materialNoTex;
    WireframeMaterial materialWireframe;
    NormalsDebugMaterial materialNormalsDebug;
    LitMaterial materialLit;

    Mesh suzanneMesh;
    Mesh ballMesh;

    Texture sampleTexture;

    // Constant Descriptor Table resources.
    //ComPtr<ID3D12DescriptorHeap> m_constantDescriptorTableHeaps[mc_frameBufferCount];
    ComPtr<ID3D12Resource> m_constantDescriptorTableBuffers[mc_frameBufferCount];
    UINT m_cbvPerFrameSize;
    ColorConstantBuffer m_cbData;
    UINT8* cbColorMultiplierGPUAddress[mc_frameBufferCount]; // Pointer to the memory location we get when we map our constant buffer.

    // Constant Root Buffers resources (for WVP matrices).
    ComPtr<ID3D12Resource> m_WVPConstantBuffers[mc_frameBufferCount];
    UINT8* m_WVPConstantBuffersGPUAddress[mc_frameBufferCount];

    // Constant Root Buffer resources (for lighting parameters).
    ComPtr<ID3D12Resource> m_LigtParamConstantBuffer;
    UINT8* m_LightParamConstantBufferGPUAddres;

    // Scene objects
    Camera m_mainCamera;
    wvpConstantBuffer m_wvpPerObject;
    lightParamsConstantBuffer lightParams;

    DirectX::XMFLOAT4X4 pyramid1WorldMat; // our first cubes world matrix (transformation matrix)
    DirectX::XMFLOAT4X4 pyramid1RotMat; // this will keep track of our rotation for the first cube
    DirectX::XMFLOAT4 pyramid1Position; // our first cubes position in space

    DirectX::XMFLOAT4X4 pyramid2WorldMat; // our first cubes world matrix (transformation matrix)
    DirectX::XMFLOAT4X4 pyramid2RotMat; // this will keep track of our rotation for the second cube
    DirectX::XMFLOAT4 pyramid2PositionOffset; // our second cube will rotate around the first cube, so this is the position offset from the first cube

    // Synchronization objects.
    UINT m_frameBufferIndex;
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence[mc_frameBufferCount];
    UINT64 m_fenceValue[mc_frameBufferCount];

    // Input related objects
    bool hasFocus;
    DirectX::XMFLOAT2 mousePos;
    DirectX::XMVECTOR mouseDelta;
    DirectX::XMFLOAT2 windowCenter;
    DirectX::XMFLOAT3 keyboradMovementInput;

    void LoadPipeline();
    void LoadAssets();
    void LoadMaterials();
    void LoadScene();
    void PopulateCommandList();
    void WaitForPreviousFrame();

    void SetLightPosition();
    void CreateSphere();
    void GenerateSphereVertices(std::vector<Vertex>& triangleVertices, std::vector<DWORD>& triangleIndices);

    void OnEarlyUpdate();
    void GetMouseDelta();
};

