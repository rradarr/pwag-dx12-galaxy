#include "stdafx.h"
#include "EngineHelpers.h"

#include "dx_includes/DXSampleHelper.h"

bool EngineHelpers::assetsPathInitialized = false;
std::wstring EngineHelpers::m_assetsPath;

std::wstring EngineHelpers::GetAssetFullPath(LPCWSTR assetName)
{
    if (!assetsPathInitialized)
    {
        WCHAR assetsPath[512];
        GetAssetsPath(assetsPath, _countof(assetsPath));
        m_assetsPath = assetsPath;
    }
    return m_assetsPath + assetName;
}
