#include "VoyagerEngine.h"

#include "TextureLoader.h"

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
    LoadScene();
    std::cout << "Engine initialized." << std::endl;
}

void VoyagerEngine::OnUpdate()
{
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
    UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
    // Enable D3D12 debug layer
    ComPtr<ID3D12Debug> debugController;
    ComPtr<ID3D12Debug1> debugController2;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        debugController->QueryInterface(IID_PPV_ARGS(&debugController2));
        debugController2->SetEnableGPUBasedValidation(true);

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

    // Create RTV descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = mc_frameBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
        // create a descriptor range (descriptor table) and fill it out
        // this is a range of descriptors inside a descriptor heap
        D3D12_DESCRIPTOR_RANGE  descriptorTableVertexRanges[1]; // only one range right now
        descriptorTableVertexRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
        descriptorTableVertexRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
        descriptorTableVertexRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
        descriptorTableVertexRanges[0].RegisterSpace = 0; // space 0. can usually be zero
        descriptorTableVertexRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

        // create a descriptor table for the vertex stage
        D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableVertex;
        descriptorTableVertex.NumDescriptorRanges = _countof(descriptorTableVertexRanges);
        descriptorTableVertex.pDescriptorRanges = &descriptorTableVertexRanges[0]; // the pointer to the beginning of our ranges array

        // Create the descriptor range for the texture resource (it needs to be a separate root parameter as it's used by a different shader stage!)
        D3D12_DESCRIPTOR_RANGE  descriptorTablePixelRanges[1];
        descriptorTablePixelRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorTablePixelRanges[0].NumDescriptors = 1;
        descriptorTablePixelRanges[0].BaseShaderRegister = 0; // t0 in shader
        descriptorTablePixelRanges[0].RegisterSpace = 0;
        descriptorTablePixelRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // create a descriptor table for the pixel stage
        D3D12_ROOT_DESCRIPTOR_TABLE descriptorTablePixel;
        descriptorTablePixel.NumDescriptorRanges = _countof(descriptorTablePixelRanges);
        descriptorTablePixel.pDescriptorRanges = &descriptorTablePixelRanges[0]; // the pointer to the beginning of our ranges array

        // Create the root descriptor (for wvp matrices)
        D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
        rootCBVDescriptor.ShaderRegister = 1; // b1 in shader
        rootCBVDescriptor.RegisterSpace = 0;

        // create a root parameter and fill it out
        D3D12_ROOT_PARAMETER  rootParameters[3];
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
        rootParameters[0].DescriptorTable = descriptorTableVertex; // this is our descriptor table for this root parameter
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // vertex will access cbv

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor = rootCBVDescriptor;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
        rootParameters[2].DescriptorTable = descriptorTablePixel; // this is our descriptor table for this root parameter
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // pixel will access srv (texture)

        // Create the static sample description.
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0; // s0 in shader
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        // Root parameters (first argument) can be root constants, root descriptors or descriptor tables.
        rootSignatureDesc.Init(
            _countof(rootParameters),
            rootParameters,
            1,
            &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

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
        if (FAILED(D3DCompileFromFile(GetAssetFullPath(L"VertexShader.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, vertexShaderErrors.GetAddressOf())))
        {
            OutputDebugStringA((char *)vertexShaderErrors->GetBufferPointer());
            shaderCompilationFailed = true;
        }

        if (FAILED(D3DCompileFromFile(GetAssetFullPath(L"PixelShader.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, pixelShaderErrors.GetAddressOf())))
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
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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

    // Create the buffers for use with the root signature
    {
        // Create our main SRV/CBV/UAV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = mc_frameBufferCount + 1; // Amount of all used descriptors (cbv per frame + one texture)
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_shaderAccessHeap)));
        m_shaderAccessHeap->SetName(L"Main Shader Access Descriptor Heap");

        m_shaderAccessDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_shaderAccessHeapHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_shaderAccessHeap->GetCPUDescriptorHandleForHeapStart());
        m_shaderAccessHeapHeadHandle = m_shaderAccessHeapHandle;

        // Create the constant buffer commited resource.
        // We will update the constant buffer one or more times per frame, so we will use only an upload heap
        // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
        // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
        // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
        // will be modified and uploaded at least once per frame, so we only use an upload heap.

        // Create the resources as commited resources, and pointers to cbv for each frame
        for (int i = 0; i < mc_frameBufferCount; ++i)
        {
            // Size of the resource heap must be a multiple of 64KB for single-textures and constant buffers
            // Will be data that is read from so we keep it in the generic read state.
            AllocateBuffer(m_constantDescriptorTableBuffers[i], 1042 * 64, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            m_constantDescriptorTableBuffers[i]->SetName(L"Constant Buffer Table Buffer Upload Resource Heap");

            m_cbvPerFrameSize = (sizeof(ColorConstantBuffer) + 255) & ~255;

            // Create the cbviews.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_constantDescriptorTableBuffers[i]->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = m_cbvPerFrameSize;    // CB size is required to be 256-byte aligned.

            m_device->CreateConstantBufferView(&cbvDesc, m_shaderAccessHeapHeadHandle);
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

        for (int i = 0; i < mc_frameBufferCount; ++i)
        {
            // create resource for cube 1
            // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            AllocateBuffer(m_constantRootDescriptorBuffers[i], 1024 * 64, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
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
        // Create the vertex buffer
        {
            // Define the geometry vertices.
            Vertex triangleVertices[] =
            {
                { { -0.25f, -0.25f, -0.25f }, { 1.0f, 0.0f, 0.0f, 1.0f }, {0.f, 0.f} }, // 0 BASE BACK LEFT
                { { 0.25f, -0.25f, -0.25f }, { 0.0f, 1.0f, 0.0f, 1.0f }, {1.f, 0.f} },  // 1 BASE BACK RIGHT
                { { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f, 1.0f }, {1.f, 1.f} },   // 2 BASE FRONT RIGHT
                { { -0.25f, -0.25f, 0.25f }, { 1.0f, 0.0f, 0.0f, 1.0f }, {0.f, 1.f} },   // 3 BASE FRONT LEFT
                { { 0.f, 0.25f, 0.f }, { 0.0f, 1.0f, 0.0f, 1.0f }, {0.5f, 0.5f} }         // 4 TOP
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

        // Load the texture
        {
            // We will need a command list for the texture upload, so we need to reset it
            // (after using it and waiting on it during index and vertex buffer uploads).
            ThrowIfFailed(m_commandAllocator[m_frameBufferIndex]->Reset());
            ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameBufferIndex].Get(), m_pipelineState.Get()));

            TextureLoader textureLoader;
            D3D12_RESOURCE_DESC textureDesc;
            int imageBytesPerRow;
            BYTE* imageData;
            int imageSize = textureLoader.LoadTextureFromFile(&imageData, textureDesc, GetAssetFullPath(L"texture.png").c_str(), imageBytesPerRow);

            CD3DX12_RESOURCE_DESC desc(textureDesc);
            UINT64 textureUploadBufferSize = 0;
            m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
            ComPtr<ID3D12Resource> textureUploadBuffer;
            AllocateBuffer(m_textureBuffer, &desc, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);
            AllocateBuffer(textureUploadBuffer, textureUploadBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &imageData[0];
            textureData.RowPitch = imageBytesPerRow;
            textureData.SlicePitch = imageBytesPerRow * textureDesc.Height;
            FillBuffer(m_textureBuffer, textureData, textureUploadBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_textureBuffer->SetName(L"Texture buffer resource");

            // Create the SRV Descriptor Heap
            /*D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = 1;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SRVHeap)));
            m_SRVHeap->SetName(L"SRV Heap");*/

            // Create the SRV view/descriptor
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            m_device->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc, m_shaderAccessHeapHeadHandle);
            m_shaderAccessHeapHeadHandle = m_shaderAccessHeapHeadHandle.Offset(1, m_shaderAccessDescriptorSize);
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
            AllocateBuffer(m_depthStencilBuffer, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_HEAP_TYPE_DEFAULT, &depthOptimizedClearValue);
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

void VoyagerEngine::LoadScene()
{
    m_mainCamera.InitCamera(DirectX::XMFLOAT4(0.f, 0.f, -4.f, 0.f), DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f), aspectRatio);

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
}

void VoyagerEngine::PopulateCommandList()
{
    // We already waited for the GPU to finish work for this framebuffer (we cannot reset a commandAllocator before it's commands have finished executing!)
    ThrowIfFailed(m_commandAllocator[m_frameBufferIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameBufferIndex].Get(), m_pipelineState.Get()));

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_frameBufferIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsHeap->GetCPUDescriptorHandleForHeapStart());
    // set the render target for the output merger stage (the output of the pipeline)
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);

    // set constant buffer descriptor table heap and srv descriptor table heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_shaderAccessHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    // set the root descriptor table 0 to the constant buffer descriptor heap
    CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(m_shaderAccessHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle.Offset(m_frameBufferIndex, m_shaderAccessDescriptorSize));

    //ID3D12DescriptorHeap* srvDescriptorHeaps[] = { m_SRVHeap.Get() };
    //m_commandList->SetDescriptorHeaps(_countof(srvDescriptorHeaps), srvDescriptorHeaps);
    // set the root descriptor table 2 to the srv
    descriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_shaderAccessHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(2, descriptorHandle.Offset(3, m_shaderAccessDescriptorSize));

    // draw first pyramid
    m_commandList->SetGraphicsRootConstantBufferView(1, m_constantRootDescriptorBuffers[m_frameBufferIndex]->GetGPUVirtualAddress());
    m_commandList->DrawIndexedInstanced(3 * 6, 1, 0, 0, 0);

    // draw second pyramid
    m_commandList->SetGraphicsRootConstantBufferView(1, m_constantRootDescriptorBuffers[m_frameBufferIndex]->GetGPUVirtualAddress() + sizeof(wvpConstantBuffer));
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

void VoyagerEngine::AllocateBuffer(ComPtr<ID3D12Resource>& bufferResource, CD3DX12_RESOURCE_DESC* resourceDescriptor, D3D12_RESOURCE_STATES bufferState, D3D12_HEAP_TYPE heapType, D3D12_CLEAR_VALUE* optimizedClearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        resourceDescriptor,
        bufferState,
        optimizedClearValue,
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
