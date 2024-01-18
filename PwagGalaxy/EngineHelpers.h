#pragma once
#include <string>

class EngineHelpers
{
public:
    static std::wstring GetAssetFullPath(LPCWSTR assetName);

private:
    // Root assets path.
    static std::wstring m_assetsPath;
    static bool assetsPathInitialized;
};
