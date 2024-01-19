#pragma once

#include <unordered_map>

class Texture;

class ResourceManager
{
public:
    ResourceManager() = default;
private:
    std::unordered_map<std::string, Texture> textures;
};

