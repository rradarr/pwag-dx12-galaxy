#include "stdafx.h"
#include "AssetConfigReader.h"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include <fstream>
#include "EngineHelpers.h"

bool AssetConfigReader::ReadJson(const std::string fileName)
{
    std::wstring tmpName(fileName.begin(), fileName.end());
    LPCWSTR wideFileName = tmpName.c_str();
    std::wstring inputFileWide = { EngineHelpers::GetAssetFullPath(wideFileName).c_str() };
    std::string inputFile = { inputFileWide.begin(), inputFileWide.end() };

    std::ifstream inputJsonFileStream(inputFile);
    if (!inputJsonFileStream.is_open())
    {
        return false;
    }
    rapidjson::IStreamWrapper inputJsonStreamWrapper(inputJsonFileStream);

    config.ParseStream(inputJsonStreamWrapper);

    assert(config.IsObject());

    for (auto& v : config["textures"].GetArray())
    {
        std::cout << v["name"].GetString() << std::endl;
    }


    return true;
}
