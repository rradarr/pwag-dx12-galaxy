#pragma once

#include "Engine.h"

class VoyagerEngine : public Engine
{
public:
    VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
};

