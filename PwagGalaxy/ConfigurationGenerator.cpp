#include "stdafx.h"
#include "ConfigurationGenerator.h"

#include <iomanip>
#include <sstream>
#include <string>

std::string ConfigurationGenerator::GenerateSeed(std::string key) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(key);

    std::stringstream ss;
    ss << std::hex << hashValue;
    std::string hashString = ss.str();

    while (hashString.length() < 64) {

        ss << std::hex << hasher(hashString + key);
        hashString = ss.str();
    }
    if (hashString.length() > 64) {
        hashString = hashString.substr(hashString.length() - 64);
    }

    return hashString;
}

PlanetConfiguration ConfigurationGenerator::GeneratePlanetConfiguration(std::string id, float orbit, DirectX::XMFLOAT3 starPosition) {
	PlanetConfiguration planetConfiguration;


	planetConfiguration.id = generalSettings.planetCode + id;
    planetConfiguration.seed = GenerateSeed(id);

    std::vector<float> probability = GenerateProbabilityTable(planetConfiguration.seed);
    planetConfiguration.exist = probability[0] < planetSettings.probability;
    planetConfiguration.radius = EstimateValue(planetSettings.radius, probability[1]);
    planetConfiguration.velocity = EstimateValue(planetSettings.velocity, probability[2]);
    planetConfiguration.orbitOffset = EstimateValue(planetSettings.orbitDescriptor.offset, probability[3]);
    planetConfiguration.orbitInitialAngleRad = EstimateValue(planetSettings.orbitDescriptor.initAngleRad, probability[4]);
    planetConfiguration.orbitEmptyRange = EstimateValue(planetSettings.orbitDescriptor.emptyRange, probability[5]);
    planetConfiguration.orbitAxis = DirectX::XMFLOAT3(0, 0, 0);
    planetConfiguration.orbitAxis.x = probability[6];
    planetConfiguration.orbitAxis.y = probability[7];

    std::string seed = GenerateSeed(planetConfiguration.seed.substr(56, 8));
    probability = GenerateProbabilityTable(seed);
    planetConfiguration.orbitAxis.z = probability[0];
    planetConfiguration.orbitAngle = EstimateValue(planetSettings.orbitDescriptor.rotationAngle, probability[1]);
    planetConfiguration.numberOfLayers = EstimateValue(planetSettings.numberOfLayers, probability[2]);
    planetConfiguration.orbit = orbit + planetConfiguration.orbitEmptyRange + planetConfiguration.radius;
    planetConfiguration.starPosition = starPosition;

    for (int i = 0; i < planetConfiguration.numberOfLayers; i++) {

        PlanetSurfaceConfiguration surfaceConfiguration;
        seed = GenerateSeed(seed);
        probability = GenerateProbabilityTable(seed);
        surfaceConfiguration.baseRoughness = EstimateValue(planetSettings.surfaceDescriptor.baseRoughness, probability[0]);
        surfaceConfiguration.roughness = EstimateValue(planetSettings.surfaceDescriptor.roughness, probability[1]);
        surfaceConfiguration.persistance = EstimateValue(planetSettings.surfaceDescriptor.persistance, probability[2]);
        surfaceConfiguration.minValue = EstimateValue(planetSettings.surfaceDescriptor.minValue, probability[3]);
        surfaceConfiguration.strength = EstimateValue(planetSettings.surfaceDescriptor.strength, probability[4]);
        surfaceConfiguration.steps = EstimateValue(planetSettings.surfaceDescriptor.steps, probability[5]);
        surfaceConfiguration.centre = DirectX::XMFLOAT3(0, 0, 0);
        surfaceConfiguration.userFirstLayerAsMask = (i == 0) ? false : (probability[7] < planetSettings.surfaceDescriptor.maskProbability);
        planetConfiguration.layers.push_back(surfaceConfiguration);
    }

    return planetConfiguration;
}


std::vector<float> ConfigurationGenerator::GenerateProbabilityTable(const std::string& seed) {
    std::vector<float> probability(8, 0.0f);

    for (int i = 0; i < 8; i++) {
        std::string hexSubstring = seed.substr(i * 8, 8);
        std::stringstream hexStream(hexSubstring);
        uint32_t hexValue;
        hexStream >> std::hex >> hexValue;

        probability[i] = static_cast<float>(hexValue) / static_cast<float>(UINT32_MAX);
    }

    return probability;
}



float ConfigurationGenerator::EstimateValue(MinMaxRange attribute, float probability) {
    return attribute.min + (attribute.max - attribute.min) * probability;
}

int ConfigurationGenerator::EstimateValue(MinMaxRangeInt attribute, float probability){
    return attribute.min + (attribute.max - attribute.min) * probability;
}


void ConfigurationGenerator::PrintPlanetConfiguration(const PlanetConfiguration& planetConfig) {
    std::cout << "id: " << planetConfig.id << std::endl;
    std::cout << "seed: " << planetConfig.seed << std::endl;
    std::cout << "exist: " << (planetConfig.exist ? "true" : "false") << std::endl;
    std::cout << "radius: " << planetConfig.radius << std::endl;
    std::cout << "velocity: " << planetConfig.velocity << std::endl;
    std::cout << "orbitOffset: " << planetConfig.orbitOffset << std::endl;
    std::cout << "orbitInitialAngleRad: " << planetConfig.orbitInitialAngleRad << std::endl;
    std::cout << "orbitEmptyRange: " << planetConfig.orbitEmptyRange << std::endl;
    std::cout << "orbitAxis: (" << planetConfig.orbitAxis.x << ", " << planetConfig.orbitAxis.y << ", " << planetConfig.orbitAxis.z << ")" << std::endl;
    std::cout << "orbitAngle: " << planetConfig.orbitAngle << std::endl;
    std::cout << "numberOfLayers: " << planetConfig.numberOfLayers << std::endl;
    std::cout << "orbit: " << planetConfig.orbit << std::endl;
    std::cout << "starPosition: (" << planetConfig.starPosition.x << ", " << planetConfig.starPosition.y << ", " << planetConfig.starPosition.z << ")" << std::endl;

    std::cout << "Layers:" << std::endl;
    for (int i = 0; i < planetConfig.numberOfLayers; i++) {
        PlanetSurfaceConfiguration layer = planetConfig.layers[i];
        std::cout << "  Layer: "<<i<< std::endl;
        std::cout << "  Base Roughness: " << layer.baseRoughness << std::endl;
        std::cout << "  Roughness: " << layer.roughness << std::endl;
        std::cout << "  Persistance: " << layer.persistance << std::endl;
        std::cout << "  Steps: " << layer.steps << std::endl;
        std::cout << "  Centre: (" << layer.centre.x << ", " << layer.centre.y << ", " << layer.centre.z << ")" << std::endl;
        std::cout << "  Min Value: " << layer.minValue << std::endl;
        std::cout << "  Strength: " << layer.strength << std::endl;
        std::cout << "  Use First Layer As Mask: " << (layer.userFirstLayerAsMask ? "true" : "false") << std::endl;
        std::cout << std::endl;
    }

}