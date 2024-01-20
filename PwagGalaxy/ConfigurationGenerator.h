#pragma once
#include <string>

struct MinMaxRange
{
    float min;
    float max;
};

struct MinMaxRangeInt
{
    int min;
    int max;
};


struct GeneralSettings {
    std::string solarSystemCode = "SY-";
    std::string planetCode = "P-";
    std::string starCode = "S-";
};

struct PlanetSurfaceSettings {
    DirectX::XMFLOAT3 center = DirectX::XMFLOAT3(0, 0, 0);
    MinMaxRange baseRoughness = { 0.85f, 1.0f };
    MinMaxRange roughness = { 2.7f, 3.0f };
    MinMaxRange persistance = { 0.4f, 0.5f };
    MinMaxRange minValue = { 0.8f, 0.9f };
    MinMaxRange strength = { 0.1f, 0.3f };
    MinMaxRange weightMultiplayer = { 0.5f, 0.75f };
    MinMaxRangeInt steps = { 2, 4 };
    float maskProbability = 1.0f;
};

struct PlanetOrbitSettings {
    MinMaxRange offset = { -1.0f, 1.0f };
    MinMaxRange initAngleRad = { 0.0f,  2.0f * DirectX::XM_PI };
    MinMaxRange emptyRange = { 1.0f, 15.0f };
    MinMaxRange rotationAngle = { 0.0f, 360.0f };
};

struct PlanetSettings
{
    PlanetSurfaceSettings orbitDescriptor;
    PlanetOrbitSettings surfaceDescriptor;
    float probability = 0.2f;
    MinMaxRange radius = { 10.0f, 50.0f };
    MinMaxRange velocity = { 10.0f, 50.0f };
    MinMaxRangeInt numberOfLayers = { 2, 4 };
};

struct PlanetConfiguration
{
    std::string id;
    std::string seed;
    bool exist;
    float radius;
    float velocity;
    float orbitOffset;
    float orbitInitialAngleRad;
    float orbitEmptyRange;
    DirectX::XMFLOAT3 orbitAxis;
    float orbitAngle;
    int numberOfLayers;
    float orbit;
    DirectX::XMFLOAT3 starPosition;
};




class ConfigurationGenerator
{
    public:
        //PlanetConfiguration GeneratePlanetConfiguration(std::string id, float orbit, DirectX::XMFLOAT3 starPosition);
        std::string GenerateSeed(std::string key);
    private:
        GeneralSettings generalSettings;
        PlanetSettings planetSettings;

        
        
};

