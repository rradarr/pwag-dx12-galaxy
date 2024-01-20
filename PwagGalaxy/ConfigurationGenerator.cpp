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

//PlanetConfiguration ConfigurationGenerator::GeneratePlanetConfiguration(std::string id, float orbit, DirectX::XMFLOAT3 starPosition) {
//	PlanetConfiguration planetConfiguration;
//
//	planetConfiguration.id = generalSettings.planetCode + id;
//    planetConfiguration.seed;
//
//    return planetConfiguration;
//}


