#include "VoyagerEngine.h"

VoyagerEngine::VoyagerEngine(UINT windowWidth, UINT windowHeight, std::wstring windowName) :
    Engine(windowWidth, windowHeight, windowName)
{
}

void VoyagerEngine::OnInit()
{
    std::cout << "Engine initialized." << std::endl;
}

void VoyagerEngine::OnUpdate()
{
    //std::cout << "Engine updated." << std::endl;
}

void VoyagerEngine::OnRender()
{
    //std::cout << "Engine rendered." << std::endl;
}

void VoyagerEngine::OnDestroy()
{
    std::cout << "Engine destroyed." << std::endl;
}

void VoyagerEngine::OnKeyDown(UINT8 keyCode)
{
    std::cout << "Engine detected key down." << std::endl;
}

void VoyagerEngine::OnKeyUp(UINT8 keyCode)
{
    std::cout << "Engine detected key up." << std::endl;
}
