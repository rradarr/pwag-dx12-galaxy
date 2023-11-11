#include "VoyagerEngine.h"

VoyagerEngine::VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName) :
    Engine(windowWidth, windowHeight, windowName)
{
    m_frameBufferIndex = 0;
    m_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight) };
    m_scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
    m_rtvDescriptorSize = 0;
}

void VoyagerEngine::OnInit()
{
    LoadPipeline();
    LoadAssets();
    std::cout << "Engine initialized." << std::endl;
}

void VoyagerEngine::OnUpdate()
{
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
    UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
    // Enable D3D12 debug layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter; // Adapters represent the hardware of each graphics card (including integrated graphics)
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice( // The device is the logical GPU representation and interface used in DX
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

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
    ThrowIfFailed(factory->CreateSwapChain(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        &swapChainDesc,
        &swapChain
    ));

    ThrowIfFailed(swapChain.As(&m_swapChain));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(WindowsApplication::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    m_frameBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = mc_frameBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Setup RTV descriptor to specify sRGB format.
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // Create a RTV for each frame.
        for (UINT n = 0; n < mc_frameBufferCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]))); // Store pointer to swapchain buffer in m_renderTargets[n]
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvHandle); // Create a resource based on the buffer pointer (?) within the heap at rtvHandle
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    for (int i = 0; i < mc_frameBufferCount; i++) {
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i])));
    }

    std::cout << "Pipeline loaded." << std::endl;
}

void VoyagerEngine::LoadAssets()
{
    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        // Root parameters (first argument) can be a root constant, a root descriptor or a descriptor table.
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        ComPtr<ID3DBlob> vertexShaderErrors;
        ComPtr<ID3DBlob> pixelShaderErrors;
        bool shaderCompilationFailed = false;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        // When debugging, compiling at runtime provides descriptive errors. At release, precompiled shader bytecode should be loaded.
        if (FAILED(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, vertexShaderErrors.GetAddressOf())))
        {
            OutputDebugStringA((char *)vertexShaderErrors->GetBufferPointer());
            shaderCompilationFailed = true;
        }

        if (FAILED(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, pixelShaderErrors.GetAddressOf())))
        {
            OutputDebugStringA((char*)pixelShaderErrors->GetBufferPointer());
            shaderCompilationFailed = true;
        }

        if (shaderCompilationFailed)
        {
            throw "Shader compilation failed!";
        }

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Prepare the depth/stencil descpriptor for the PSO creation
        D3D12_DEPTH_STENCIL_DESC dtDesc = {};
        dtDesc.DepthEnable = TRUE;
        dtDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        dtDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // More efficient than _LESS_EQUAL as less fragments pass the test.
        dtDesc.StencilEnable = FALSE; // disable stencil test
        dtDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        dtDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
            { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        dtDesc.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
        dtDesc.BackFace = defaultStencilOp;

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
        psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // TODO: This changes between solid and wireframe.
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = dtDesc;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[0].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    //ThrowIfFailed(m_commandList->Close());

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        for (int i = 0; i < mc_frameBufferCount; i++) {
            ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i])));
            m_fenceValue[i] = 0;
        }

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    // Create the vertex and index buffers.
    {
        // Create the vertex buffer
        {
            // Define the geometry vertices.
            Vertex triangleVertices[] =
            {
                { { -0.25f, -0.25f, -0.25f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 0 BASE BACK LEFT
                { { 0.25f, -0.25f, -0.25f }, { 0.0f, 1.0f, 0.0f, 1.0f } },  // 1 BASE BACK RIGHT
                { { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f, 1.0f } },   // 2 BASE FRONT RIGHT
                { { -0.25f, -0.25f, 0.25f }, { 1.0f, 0.0f, 0.0f, 1.0f } },   // 3 BASE FRONT LEFT
                { { 0.f, 0.25f, 0.f }, { 0.0f, 1.0f, 0.0f, 1.0f } }         // 4 TOP
            };

            UINT vertexBufferSize = sizeof(triangleVertices);
            ComPtr<ID3D12Resource> vertexUploadBuffer;
            AllocateBuffer(vertexUploadBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            AllocateBuffer(m_vertexBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

            D3D12_SUBRESOURCE_DATA vertexData = {};
            vertexData.pData = reinterpret_cast<BYTE*>(triangleVertices);
            vertexData.RowPitch = vertexBufferSize;
            vertexData.SlicePitch = vertexBufferSize;

            // We don't close the commandList and wait for it to finish as we have more commands to upload.
            FillBuffer(m_vertexBuffer, vertexData, vertexUploadBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, false);

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;

        // Create the index buffer.
            // Define the geometry indices.
            DWORD triangleIndices[] =
            {
                0, 3, 1,    // BASE LEFT
                1, 3, 2,    // BASE RIGHT
                4, 0, 1,    // WALL BACK
                4, 3, 0,    // WALL LEFT
                4, 1, 2,    // WALL RIGHT
                4, 2, 3     // WALL FRONT
            };
            UINT indexBufferSize = sizeof(triangleIndices);
            ComPtr<ID3D12Resource> indexUploadBuffer;
            AllocateBuffer(indexUploadBuffer, indexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            AllocateBuffer(m_indexBuffer, indexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

            D3D12_SUBRESOURCE_DATA indexData = {};
            indexData.pData = reinterpret_cast<BYTE*>(triangleIndices);
            indexData.RowPitch = indexBufferSize;
            indexData.SlicePitch = indexBufferSize;

            FillBuffer(m_indexBuffer, indexData, indexUploadBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            // Initialize the index buffer view.
            m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            m_indexBufferView.SizeInBytes = indexBufferSize;
        }

        // Create the depth/stencil heap and buffer.
        {
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsHeap)));

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
            depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
            depthOptimizedClearValue.DepthStencil.Stencil = 0;

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            AllocateBuffer(m_depthStencilBuffer, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_HEAP_TYPE_DEFAULT, depthOptimizedClearValue);
            m_dsHeap->SetName(L"Depth/Stencil Resource Heap");

            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_dsHeap->GetCPUDescriptorHandleForHeapStart());
        }
    }

    std::cout << "Assets loaded." << std::endl;
}

void VoyagerEngine::PopulateCommandList()
{
    // We already waited for the GPU to finish work for this framebuffer (we cannot reset a commandAllocator before it's commands have finished executing!)
    ThrowIfFailed(m_commandAllocator[m_frameBufferIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameBufferIndex].Get(), m_pipelineState.Get()));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameBufferIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsHeap->GetCPUDescriptorHandleForHeapStart());
    // set the render target for the output merger stage (the output of the pipeline)
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->DrawIndexedInstanced(3 * 6, 1, 0, 0, 0);

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

void VoyagerEngine::AllocateBuffer(ComPtr<ID3D12Resource>& bufferResource, UINT bufferSize, D3D12_RESOURCE_STATES bufferState, D3D12_HEAP_TYPE heapType)
{
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        bufferState,
        nullptr,
        IID_PPV_ARGS(&bufferResource)));
}

void VoyagerEngine::AllocateBuffer(ComPtr<ID3D12Resource>& bufferResource, CD3DX12_RESOURCE_DESC* resourceDescriptor, D3D12_RESOURCE_STATES bufferState, D3D12_HEAP_TYPE heapType, D3D12_CLEAR_VALUE optimizedClearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        resourceDescriptor,
        bufferState,
        &optimizedClearValue,
        IID_PPV_ARGS(&bufferResource)));
}

/* Copy data to Default buffer with the use of an Upload buffer. Optionally will submit a command list to the gpu and wait for it to finish. */
void VoyagerEngine::FillBuffer(ComPtr<ID3D12Resource>& bufferResource, D3D12_SUBRESOURCE_DATA data, ComPtr<ID3D12Resource>& uploadBufferResource, D3D12_RESOURCE_STATES finalBufferState, bool waitForGPU)
{
    // This starts a command list and records a copy command. (The list is still recording and will need to be closed and executed).
    UpdateSubresources(m_commandList.Get(), bufferResource.Get(), uploadBufferResource.Get(), 0, 0, 1, &data);
    // Set a resource barrier to transition the buffer from OCPY_DEST to a VERTEX_AND_CONSTANT_BUFFER. (this is also a command).
    CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(bufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalBufferState);
    m_commandList->ResourceBarrier(1, &transitionBarrier);
    if (waitForGPU)
    {
        // Execute the command list.
        m_commandList->Close();
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
        m_fenceValue[m_frameBufferIndex]++;
        m_commandQueue->Signal(m_fence[m_frameBufferIndex].Get(), m_fenceValue[m_frameBufferIndex]);
        WaitForPreviousFrame();
    }
}
