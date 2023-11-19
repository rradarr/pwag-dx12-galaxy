#include "TextureLoader.h"

IWICImagingFactory* TextureLoader::wicFactory = nullptr;

TextureLoader::TextureLoader()
{
    if (wicFactory == nullptr)
    {
        // Initialize the COM library
        CoInitialize(NULL);

        // create the WIC factory
        ThrowIfFailed(CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        ));
    }
}

int TextureLoader::LoadTextureFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
    HRESULT hr;

    // reset decoder, frame and converter since these will be different for each image we load
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICFormatConverter* wicConverter = NULL;

    bool imageConverted = false;

    // load a decoder for the image
    ThrowIfFailed(wicFactory->CreateDecoderFromFilename(
        filename,                        // Image we want to load in
        NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
        GENERIC_READ,                    // We want to read from this file
        WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
        &wicDecoder                      // the wic decoder to be created
    ));

    // get image from decoder (this will decode the "frame")
    ThrowIfFailed(wicDecoder->GetFrame(0, &wicFrame));

    // get wic pixel format of image
    WICPixelFormatGUID pixelFormat;
    ThrowIfFailed(wicFrame->GetPixelFormat(&pixelFormat));

    // get size of image
    UINT textureWidth, textureHeight;
    ThrowIfFailed(wicFrame->GetSize(&textureWidth, &textureHeight));

    // TODO: we are not handling sRGB types in this!

    // convert wic pixel format to dxgi pixel format
    DXGI_FORMAT dxgiFormat = WICFormatToDXGIFormat(pixelFormat);

    // if the format of the image is not a supported dxgi format, try to convert it
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        // get a dxgi compatible wic format from the current image format
        WICPixelFormatGUID convertToPixelFormat = ConvertToCompatibleWICFormat(pixelFormat);

        // return if no dxgi compatible format was found
        if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

        // set the dxgi format
        dxgiFormat = WICFormatToDXGIFormat(convertToPixelFormat);

        // create the format converter
        ThrowIfFailed(wicFactory->CreateFormatConverter(&wicConverter));

        // make sure we can convert to the dxgi compatible format
        BOOL canConvert = FALSE;
        ThrowIfFailed(wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert));

        // do the conversion (wicConverter will contain the converted image)
        ThrowIfFailed(wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

        // this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
        imageConverted = true;
    }

    int bitsPerPixel = GetDXGIFormatPixelBitDepth(dxgiFormat); // number of bits per pixel
    bytesPerRow = (textureWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
    int imageSize = bytesPerRow * textureHeight; // total image size in bytes

    // allocate enough memory for the raw image data, and set imageData to point to that memory
    *imageData = (BYTE*)malloc(imageSize);

    // copy (decoded) raw image data into the newly allocated memory (imageData)
    if (imageConverted)
    {
        // if image format needed to be converted, the wic converter will contain the converted image
        ThrowIfFailed(wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData));
    }
    else
    {
        // no need to convert, just copy data from the wic frame
        ThrowIfFailed(wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData));
    }

    // now describe the texture with the information we have obtained from the image
    resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    resourceDescription.Width = textureWidth; // width of the texture
    resourceDescription.Height = textureHeight; // height of the texture
    resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    resourceDescription.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
    resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
    resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

    // return the size of the image. remember to delete the image once your done with it (in this tutorial once its uploaded to the gpu)
    return imageSize;
}

DXGI_FORMAT TextureLoader::WICFormatToDXGIFormat(WICPixelFormatGUID wicFormat)
{
    if (wicFormat == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (wicFormat == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (wicFormat == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (wicFormat == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (wicFormat == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (wicFormat == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (wicFormat == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    else if (wicFormat == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    else if (wicFormat == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (wicFormat == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    else if (wicFormat == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    else if (wicFormat == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    else if (wicFormat == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    else if (wicFormat == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    else if (wicFormat == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

    else return DXGI_FORMAT_UNKNOWN;
}

WICPixelFormatGUID TextureLoader::ConvertToCompatibleWICFormat(WICPixelFormatGUID wicFormat)
{
    // get a dxgi compatible wic format from another wic format
    if (wicFormat == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormat == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormat == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormat == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormat == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormat == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormat == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormat == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormat == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormat == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormat == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormat == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormat == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormat == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormat == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormat == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;

}

int TextureLoader::GetDXGIFormatPixelBitDepth(DXGI_FORMAT dxgiFormat)
{
    if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
    else return -1; // This is an error state indicating unknown dxgiFormat
}
