#pragma once

#include "rapidjson/document.h"

class AssetConfigReader
{
public:
    AssetConfigReader() = default;
    bool ReadJson(const std::string fileName);
private:
    rapidjson::Document config;
};

