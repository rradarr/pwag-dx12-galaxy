#pragma once

class Engine
{
public:
    Engine(UINT windowWidth, UINT windowHeight, std::wstring windowName);
    virtual ~Engine();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnKeyDown(UINT8 keyCode) {};
    virtual void OnKeyUp(UINT8 keyCode) {};

    UINT GetWidth() const           { return windowWidth; };
    UINT GetHeight() const          { return windowHeight; };
    std::wstring GetTitle() const   { return windowName; };

protected:
    UINT            windowWidth, windowHeight;
    float           aspectRatio;
    std::wstring    windowName;

    /*void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);*/

    std::wstring GetAssetFullPath(LPCWSTR assetName);

private:
    // Root assets path.
    std::wstring m_assetsPath;
};

