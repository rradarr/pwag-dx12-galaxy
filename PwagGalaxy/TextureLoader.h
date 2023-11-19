#pragma once

#include "stdafx.h"

#include "dx_includes/DXSampleHelper.h"

class TextureLoader
{
public:
    TextureLoader();

    int LoadTextureFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);

private:
    DXGI_FORMAT WICFormatToDXGIFormat(WICPixelFormatGUID wicFormat);
    WICPixelFormatGUID ConvertToCompatibleWICFormat(WICPixelFormatGUID wicFormat);
    int GetDXGIFormatPixelBitDepth(DXGI_FORMAT dxgiFormat);

    // we only need one instance of the imaging factory to create decoders and frames
    static IWICImagingFactory* wicFactory;
};

