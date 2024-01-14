#include "stdafx.h"
#include "DXContext.h"

#include "dx_includes/DXSampleHelper.h"

ComPtr<IDXGIFactory4> DXContext::factory;
ComPtr<ID3D12Device> DXContext::device;

ComPtr<ID3D12Device> DXContext::getDevice()
{
    if (device.Get() == nullptr) {
        createDevice();
    }
    return device;
}

ComPtr<IDXGIFactory4> DXContext::getFactory()
{
    if (factory.Get() == nullptr) {
        createFactory();
    }
    return factory;
}

void DXContext::createDevice()
{
    ComPtr<IDXGIAdapter1> hardwareAdapter; // Adapters represent the hardware of each graphics card (including integrated graphics)
    GetHardwareAdapter(getFactory().Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice( // The device is the logical GPU representation and interface used in DX
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)
    ));
}

void DXContext::createFactory()
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

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
}

_Use_decl_annotations_
void DXContext::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}
