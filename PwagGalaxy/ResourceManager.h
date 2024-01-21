#pragma once

#include <unordered_map>
#include <memory>

class Texture;
class Material;

class ResourceManager
{
public:
    ResourceManager() = default;
private:
    std::unordered_map<std::string, Texture> textures;
    std::unordered_map<std::string, std::unique_ptr<Material>> materials;
    //std::unordered_map<std::string, SceneObject> objects;
};

