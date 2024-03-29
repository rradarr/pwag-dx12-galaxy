#include "stdafx.h"
#include "Engine.h"


#include "dx_includes/DXSampleHelper.h"

Engine::Engine(UINT windowWidth, UINT windowHeight, std::wstring windowName) :
    windowWidth(windowWidth), windowHeight(windowHeight), windowName(windowName)
{
    aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}

Engine::~Engine()
{
}
