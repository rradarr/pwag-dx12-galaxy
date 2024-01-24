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
    MinMaxRange offset = { -0.01f, 0.01f };
    MinMaxRange initAngleRad = { 0.0f,  2.0f * DirectX::XM_PI };
    MinMaxRange emptyRange = { 0.01f, 0.15f };
    MinMaxRange rotationAngle = { -360.0f, 360.0f };
};

struct PlanetSettings
{
    PlanetSurfaceSettings surfaceDescriptor;
    PlanetOrbitSettings orbitDescriptor;
    float probability = 0.2f;
    MinMaxRange radius = { 0.1f, 0.5f };
    MinMaxRange velocity = { 0.0000005f, 0.00005f };
    MinMaxRangeInt numberOfLayers = { 2, 4 };
};


struct PlanetSurfaceConfiguration {
    float baseRoughness;
    float roughness;
    float persistance;
    int steps;
    DirectX::XMFLOAT3 centre;
    float minValue;
    float strength;
    bool userFirstLayerAsMask = false;
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
    std::vector<PlanetSurfaceConfiguration> layers;
};




class ConfigurationGenerator
{
    public:
        PlanetConfiguration GeneratePlanetConfiguration(std::string id, float orbit, DirectX::XMFLOAT3 starPosition);
        std::string GenerateSeed(std::string key);
        void PrintPlanetConfiguration(const PlanetConfiguration& planetConfig);
    private:
        GeneralSettings generalSettings;
        PlanetSettings planetSettings;
        std::vector<float> GenerateProbabilityTable(const std::string& seed);
        float EstimateValue(MinMaxRange attribute, float probability);
        int EstimateValue(MinMaxRangeInt attribute, float probability);
        

       
    
};

