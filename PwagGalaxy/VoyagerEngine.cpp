#include "stdafx.h"
#include "VoyagerEngine.h"

#include "dx_includes/DXSampleHelper.h"
#include "TextureLoader.h"
#include "WindowsApplication.h"
#include "Timer.h"
#include "DXContext.h"
#include "BufferMemoryManager.h"
#include "EngineHelpers.h"
#include "AssetConfigReader.h"

VoyagerEngine::VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName) :
    Engine(windowWidth, windowHeight, windowName)
{
    m_frameBufferIndex = 0;
    m_frameIndex = 0;
    m_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight) };
    m_scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
    m_rtvDescriptorSize = 0;
}

void VoyagerEngine::OnInit()
{
    LoadPipeline();
    LoadAssets();
    LoadScene();
    std::cout << "Engine initialized." << std::endl;
}

void VoyagerEngine::OnUpdate()
{
    OnEarlyUpdate();

    // update app logic, such as moving the camera or figuring out what objects are in view
    static float rIncrement = 0.002f;
    static float gIncrement = 0.006f;
    static float bIncrement = 0.009f;

    m_cbData.colorMultiplier.x += rIncrement;
    m_cbData.colorMultiplier.y += gIncrement;
    m_cbData.colorMultiplier.z += bIncrement;

    if (m_cbData.colorMultiplier.x >= 1.0 || m_cbData.colorMultiplier.x <= 0.0)
    {
        m_cbData.colorMultiplier.x = m_cbData.colorMultiplier.x >= 1.0 ? 1.0 : 0.0;
        rIncrement = -rIncrement;
    }
    if (m_cbData.colorMultiplier.y >= 1.0 || m_cbData.colorMultiplier.y <= 0.0)
    {
        m_cbData.colorMultiplier.y = m_cbData.colorMultiplier.y >= 1.0 ? 1.0 : 0.0;
        gIncrement = -gIncrement;
    }
    if (m_cbData.colorMultiplier.z >= 1.0 || m_cbData.colorMultiplier.z <= 0.0)
    {
        m_cbData.colorMultiplier.z = m_cbData.colorMultiplier.z >= 1.0 ? 1.0 : 0.0;
        bIncrement = -bIncrement;
    }

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbColorMultiplierGPUAddress[m_frameBufferIndex], &m_cbData, sizeof(m_cbData));

    // Update object positions.
    // create rotation matrices
    DirectX::XMMATRIX rotXMat = DirectX::XMMatrixRotationX(0.f);
    DirectX::XMMATRIX rotYMat = DirectX::XMMatrixRotationY(0.02f);
    DirectX::XMMATRIX rotZMat = DirectX::XMMatrixRotationZ(0.f);

    // add rotation to cube1's rotation matrix and store it
    DirectX::XMMATRIX rotMat = DirectX::XMLoadFloat4x4(&pyramid1RotMat) * rotXMat * rotYMat * rotZMat;
    DirectX::XMStoreFloat4x4(&pyramid1RotMat, rotMat);

    // create translation matrix for cube 1 from cube 1's position vector
    DirectX::XMMATRIX translationMat = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&pyramid1Position));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    DirectX::XMMATRIX worldMat = rotMat * translationMat;

    // store cube1's world matrix
    DirectX::XMStoreFloat4x4(&pyramid1WorldMat, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    DirectX::XMMATRIX viewMat = DirectX::XMLoadFloat4x4(&m_mainCamera.viewMat); // load view matrix
    DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&m_mainCamera.projMat); // load projection matrix
    DirectX::XMMATRIX wvpMat = DirectX::XMLoadFloat4x4(&pyramid1WorldMat) * viewMat * projMat; // create wvp matrix
    DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    DirectX::XMStoreFloat4x4(&m_wvpPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer
    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[m_frameBufferIndex], &m_wvpPerObject, sizeof(m_wvpPerObject.wvpMat));

    // now do cube2's world matrix
    // create rotation matrices for piramid2
    rotXMat = DirectX::XMMatrixRotationX(0.f);
    rotYMat = DirectX::XMMatrixRotationY(0.01f);
    rotZMat = DirectX::XMMatrixRotationZ(0.f);

    // add rotation to cube2's rotation matrix and store it
    rotMat = rotZMat * (DirectX::XMLoadFloat4x4(&pyramid2RotMat) * (rotXMat * rotYMat));
    DirectX::XMStoreFloat4x4(&pyramid2RotMat, rotMat);

    // create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
    DirectX::XMMATRIX translationOffsetMat = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&pyramid2PositionOffset));

    // we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
    DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);

    // reuse worldMat.
    // first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
    // then we translate it.
    // then we rotate it. rotation always rotates around point 0,0,0
    // finally we move it to cube 1's position, which will cause it to rotate around cube 1
    worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

    wvpMat = DirectX::XMLoadFloat4x4(&pyramid2WorldMat) * viewMat * projMat; // create wvp matrix
    transposed = DirectX::XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    DirectX::XMStoreFloat4x4(&m_wvpPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[m_frameBufferIndex] + sizeof(m_wvpPerObject), &m_wvpPerObject, sizeof(m_wvpPerObject.wvpMat));

    // store cube2's world matrix
    DirectX::XMStoreFloat4x4(&pyramid2WorldMat, worldMat);

    //std::cout << "Engine updated." << std::endl;
}

void VoyagerEngine::OnRender()
{
    m_frameBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    WaitForPreviousFrame();

    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    // Set the signall fence command at the end of the command queue (it will set the value of the fence to the passed m_fenceValue).
    m_fenceValue[m_frameBufferIndex]++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence[m_frameBufferIndex].Get(), m_fenceValue[m_frameBufferIndex]));

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(0, 0));

    //WaitForPreviousFrame();

    //std::cout << "Engine rendered." << std::endl;
}

void VoyagerEngine::OnDestroy()
{
    // Get swapchain out out fullscreen before exiting
    BOOL fs = false;
    if (m_swapChain->GetFullscreenState(&fs, NULL))
        m_swapChain->SetFullscreenState(false, NULL);

    // Wait for the GPU to be done with all frames.
    //WaitForPreviousFrame();
    for (int i = 0; i < mc_frameBufferCount; i++) {
        m_frameBufferIndex = i;
        WaitForPreviousFrame();
    }

    CloseHandle(m_fenceEvent);

    std::cout << "Engine destroyed." << std::endl;
}

void VoyagerEngine::OnKeyDown(UINT8 keyCode)
{
    std::cout << "Engine detected key down." << std::endl;
}

void VoyagerEngine::OnKeyUp(UINT8 keyCode)
{
    std::cout << "Engine detected key up." << std::endl;
}

void VoyagerEngine::LoadPipeline()
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = mc_frameBufferCount;
    swapChainDesc.BufferDesc.Width = windowWidth;
    swapChainDesc.BufferDesc.Height = windowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = WindowsApplication::GetHwnd();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(DXContext::getFactory().Get()->CreateSwapChain(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        &swapChainDesc,
        &swapChain
    ));

    ThrowIfFailed(swapChain.As(&m_swapChain));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(DXContext::getFactory().Get()->MakeWindowAssociation(WindowsApplication::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    m_frameBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create RTV descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = mc_frameBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(DXContext::getDevice().Get()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

        m_rtvDescriptorSize = DXContext::getDevice().Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

        // Setup RTV descriptor to specify sRGB format.
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // Create a RTV for each frame.
        for (UINT n = 0; n < mc_frameBufferCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]))); // Store pointer to swapchain buffer in m_renderTargets[n]
            DXContext::getDevice().Get()->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvHandle); // Create a resource based on the buffer pointer (?) within the heap at rtvHandle
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    for (int i = 0; i < mc_frameBufferCount; i++) {
        ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i])));
    }

    std::cout << "Pipeline loaded." << std::endl;
}

void VoyagerEngine::LoadAssets()
{
    defaultMaterial.SetShaders("VertexShader.hlsl", "PixelShader.hlsl");
    defaultMaterial.CreateMaterial();

    materialNoTex.SetShaders("VertexShader_noTex.hlsl", "PixelShader_noTex.hlsl");
    materialNoTex.CreateMaterial();

    // Create the command list.
    ThrowIfFailed(DXContext::getDevice().Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        for (int i = 0; i < mc_frameBufferCount; i++) {
            ThrowIfFailed(DXContext::getDevice().Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i])));
            m_fenceValue[i] = 0;
        }

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    // Create the buffers for use with the root signature
    {
        // Create our main SRV/CBV/UAV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = mc_frameBufferCount + 1; // Amount of all used descriptors (cbv per frame + one texture)
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(DXContext::getDevice().Get()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_shaderAccessHeap)));
        m_shaderAccessHeap->SetName(L"Main Shader Access Descriptor Heap");

        m_shaderAccessDescriptorSize = DXContext::getDevice().Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_shaderAccessHeapHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_shaderAccessHeap->GetCPUDescriptorHandleForHeapStart());
        m_shaderAccessHeapHeadHandle = m_shaderAccessHeapHandle;

        // Create the constant buffer commited resource.
        // We will update the constant buffer one or more times per frame, so we will use only an upload heap
        // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
        // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
        // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
        // will be modified and uploaded at least once per frame, so we only use an upload heap.

        BufferMemoryManager buffMng;

        // Create the resources as commited resources, and pointers to cbv for each frame
        for (int i = 0; i < mc_frameBufferCount; ++i)
        {
            // Size of the resource heap must be a multiple of 64KB for single-textures and constant buffers
            // Will be data that is read from so we keep it in the generic read state.
            buffMng.AllocateBuffer(m_constantDescriptorTableBuffers[i], 1042 * 64, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            m_constantDescriptorTableBuffers[i]->SetName(L"Constant Buffer Table Buffer Upload Resource Heap");

            m_cbvPerFrameSize = (sizeof(ColorConstantBuffer) + 255) & ~255;

            // Create the cbviews.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_constantDescriptorTableBuffers[i]->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = m_cbvPerFrameSize;    // CB size is required to be 256-byte aligned.

            DXContext::getDevice().Get()->CreateConstantBufferView(&cbvDesc, m_shaderAccessHeapHeadHandle);
            m_shaderAccessHeapHeadHandle.Offset(1, m_shaderAccessDescriptorSize);
            std::cout << m_shaderAccessHeapHeadHandle.ptr << std::endl;
        }
        ZeroMemory(&m_cbData, sizeof(m_cbData)); // Zero out the memory of our data.

        // Map the GPU memory of the constant buffer to a CPU-accesible region.
        CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
        for (int i = 0; i < mc_frameBufferCount; ++i)
        {
            m_constantDescriptorTableBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbColorMultiplierGPUAddress[i]));
            memcpy(cbColorMultiplierGPUAddress[i], &m_cbData, sizeof(m_cbData));
        }
    }
    // Create the buffers for wvp martices
    {
        // create the constant buffer resource heap
        // We will update the constant buffer one or more times per frame, so we will use only an upload heap
        // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
        // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
        // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
        // will be modified and uploaded at least once per frame, so we only use an upload heap

        // first we will create a resource heap (upload heap) for each frame for the models constant buffers
        // As you can see, we are allocating 64KB for each resource we create. Buffer resource heaps must be
        // an alignment of 64KB. We are creating 3 resources, one for each frame. Each constant buffer is
        // only a 4x4 matrix of floats in this tutorial. So with a float being 4 bytes, we have
        // 16 floats in one constant buffer, and we will store 2 constant buffers in each
        // heap, one for each cube, thats only 64x2 bits, or 128 bits we are using for each
        // resource, and each resource must be at least 64KB (65536 bits)
        // Create our Constant Buffer descriptor heap

        BufferMemoryManager buffMng;

        for (int i = 0; i < mc_frameBufferCount; ++i)
        {
            // create resource for cube 1
            // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            buffMng.AllocateBuffer(m_constantRootDescriptorBuffers[i], 1024 * 64, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            m_constantRootDescriptorBuffers[i]->SetName(L"Root Constant Buffer Upload Resource Heap");

            ZeroMemory(&m_wvpPerObject, sizeof(m_wvpPerObject));

            CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
            // map the resource heap to get a gpu virtual address to the beginning of the heap
            ThrowIfFailed(m_constantRootDescriptorBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[i])));

            // Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
            // so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
            memcpy(cbvGPUAddress[i], &m_wvpPerObject, sizeof(m_wvpPerObject.wvpMat)); // cube1's constant buffer data
            memcpy(cbvGPUAddress[i] + sizeof(m_wvpPerObject), &m_wvpPerObject, sizeof(m_wvpPerObject.wvpMat)); // cube2's constant buffer data
        }
    }

    // Create the vertex and index buffers.
    {
        CreateSphere();
        suzanneMesh.CreateFromFile("suzanne.obj");

        // Load the texture
        {
            sampleTexture.CreateFromFile("texture.png"); // Create the texture from file.
            sampleTexture.CreateTextureView(m_shaderAccessHeapHeadHandle); // Add the texture resource view to the heap.
            m_shaderAccessHeapHeadHandle = m_shaderAccessHeapHeadHandle.Offset(1, m_shaderAccessDescriptorSize); // Update the heap HEAD pointer/handle.
        }

        // Create the depth/stencil heap and buffer.
        {
            BufferMemoryManager buffMng;

            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(DXContext::getDevice().Get()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsHeap)));

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
            depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
            depthOptimizedClearValue.DepthStencil.Stencil = 0;

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            buffMng.AllocateBuffer(m_depthStencilBuffer, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_HEAP_TYPE_DEFAULT, &depthOptimizedClearValue);
            m_dsHeap->SetName(L"Depth/Stencil Resource Heap");

            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            DXContext::getDevice().Get()->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_dsHeap->GetCPUDescriptorHandleForHeapStart());
        }
    }

    std::cout << "Assets loaded." << std::endl;
}

void VoyagerEngine::LoadScene()
{
    m_mainCamera.InitCamera(DirectX::XMFLOAT4(0.f, 0.f, -8.f, 0.f), DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f), aspectRatio);

    //position piramids
    pyramid1Position = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR posVec = DirectX::XMLoadFloat4(&pyramid1Position);
    DirectX::XMMATRIX tmpMat = DirectX::XMMatrixTranslationFromVector(posVec);
    DirectX::XMStoreFloat4x4(&pyramid1WorldMat, tmpMat);
    DirectX::XMStoreFloat4x4(&pyramid1RotMat, DirectX::XMMatrixIdentity());

    pyramid2PositionOffset = DirectX::XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
    posVec = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&pyramid1Position), DirectX::XMLoadFloat4(&pyramid2PositionOffset));
    tmpMat = DirectX::XMMatrixTranslationFromVector(posVec);
    DirectX::XMStoreFloat4x4(&pyramid2WorldMat, tmpMat);
    DirectX::XMStoreFloat4x4(&pyramid2RotMat, DirectX::XMMatrixIdentity());

    AssetConfigReader reader;
    bool result = reader.ReadJson("assets.json");
    if (result)
    {
        std::cout << "JSON 'assets.json' read successfully." << std::endl;
    }
    else
    {
        std::cout << "Could not read JSON 'assets.json'." << std::endl;
    }
}

void VoyagerEngine::PopulateCommandList()
{
    // We already waited for the GPU to finish work for this framebuffer (we cannot reset a commandAllocator before it's commands have finished executing!)
    ThrowIfFailed(m_commandAllocator[m_frameBufferIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameBufferIndex].Get(), defaultMaterial.GetPSO().Get()));

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_frameBufferIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsHeap->GetCPUDescriptorHandleForHeapStart());
    // set the render target for the output merger stage (the output of the pipeline)
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(defaultMaterial.GetRootSignature().Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // set constant buffer descriptor table heap and srv descriptor table heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_shaderAccessHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    // set the root descriptor table 0 to the constant buffer descriptor heap
    CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(m_shaderAccessHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle.Offset(m_frameBufferIndex, m_shaderAccessDescriptorSize));

    // set the root descriptor table 2 to the srv
    descriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_shaderAccessHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(2, descriptorHandle.Offset(3, m_shaderAccessDescriptorSize));

    // draw suzanne
    suzanneMesh.InsertBufferBind(m_commandList);
    m_commandList->SetGraphicsRootConstantBufferView(1, m_constantRootDescriptorBuffers[m_frameBufferIndex]->GetGPUVirtualAddress() + sizeof(wvpConstantBuffer));
    suzanneMesh.InsertDrawIndexed(m_commandList);

    m_commandList->SetPipelineState(materialNoTex.GetPSO().Get());
    m_commandList->SetGraphicsRootSignature(materialNoTex.GetRootSignature().Get());
    descriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_shaderAccessHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle.Offset(m_frameBufferIndex, m_shaderAccessDescriptorSize));
    // draw ball
    ballMesh.InsertBufferBind(m_commandList);
    m_commandList->SetGraphicsRootConstantBufferView(1, m_constantRootDescriptorBuffers[m_frameBufferIndex]->GetGPUVirtualAddress());
    ballMesh.InsertDrawIndexed(m_commandList);

    // Indicate that the back buffer will now be used to present.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(m_commandList->Close());
}

void VoyagerEngine::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. More advanced samples
    // illustrate how to use fences for efficient resource usage.                   - So says Microsoft ~_~

    // Wait until the previous frame is finished. If the value of the fence associated with the current framebuffer is
    // less than the current m_fenceValue for the framebuffer, we know that the queue where the fence is has not finished
    // (at the end of the queue the fence will be set to the m_fenceValue++).
    if (m_fence[m_frameBufferIndex]->GetCompletedValue() < m_fenceValue[m_frameBufferIndex])
    {
        // The m_fenceEvent will trigger when the fence for the current frame buffer reaches the specified value.
        ThrowIfFailed(m_fence[m_frameBufferIndex]->SetEventOnCompletion(m_fenceValue[m_frameBufferIndex], m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void VoyagerEngine::CreateSphere()
{
    std::vector<Vertex> triangleVertices;
    std::vector<DWORD> triangleIndices;
    GenerateSphereVertices(triangleVertices, triangleIndices);
    ballMesh = Mesh(triangleVertices, triangleIndices);
}

void VoyagerEngine::GenerateSphereVertices(std::vector<Vertex>& triangleVertices, std::vector<DWORD>& triangleIndices)
{
    Vertex vert;
    vert.color = DirectX::XMFLOAT4(1, 1, 1, 1);
    vert.position.x = -1; // lower left back    0
    vert.position.y = -1;
    vert.position.z = 1;
    triangleVertices.push_back(vert);

    vert.position.x = 1; // lower right back    1
    vert.position.y = -1;
    vert.position.z = 1;
    triangleVertices.push_back(vert);

    vert.position.x = 1; // lower right front   2
    vert.position.y = -1;
    vert.position.z = -1;
    triangleVertices.push_back(vert);

    vert.position.x = -1; // lower left front   3
    vert.position.y = -1;
    vert.position.z = -1;
    triangleVertices.push_back(vert);

    vert.position.x = -1; // upper left back    4
    vert.position.y = 1;
    vert.position.z = 1;
    triangleVertices.push_back(vert);

    vert.position.x = 1; // upper right back    5
    vert.position.y = 1;
    vert.position.z = 1;
    triangleVertices.push_back(vert);

    vert.position.x = 1; // upper right front   6
    vert.position.y = 1;
    vert.position.z = -1;
    triangleVertices.push_back(vert);

    vert.position.x = -1; // upper left front   7
    vert.position.y = 1;
    vert.position.z = -1;
    triangleVertices.push_back(vert);

    // Indexes
    triangleIndices = std::vector<DWORD>{
        0, 1, 2,
        2, 3, 0, // bottom
        4, 5, 6,
        6, 7, 4, // top
        6, 5, 1,
        1, 2, 6, // right
        4, 7, 3,
        3, 0, 4, // left
        7, 6, 2,
        2, 3, 7, // front
        4, 0, 1,
        1, 5, 4  // back
        };

    return;
}

void VoyagerEngine::OnEarlyUpdate()
{
    m_frameIndex++;

    // Update the timer values (deltaTime, FPS).
    Timer* timer = Timer::GetInstance();
    timer->Update();

    // Update FPS display every half a second.
    static double timeTillFpsDisplayUpdate = 0.0;
    timeTillFpsDisplayUpdate += timer->GetDeltaTime();
    if (timeTillFpsDisplayUpdate > 0.5)
    {
        timeTillFpsDisplayUpdate = 0.0;
        std::cout << timer->GetFps() << "\r";
    }
}
