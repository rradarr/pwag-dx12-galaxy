#pragma once

//#include "stdafx.h"

using Microsoft::WRL::ComPtr;

static class DXContext
{
public:
    static ComPtr<ID3D12Device> getDevice();
    static ComPtr<IDXGIFactory4> getFactory();
private:
    static void createDevice();
    static void createFactory();
    static void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    static ComPtr<IDXGIFactory4> factory;
    static ComPtr<ID3D12Device> device;
};

