#include "stdafx.h"

#include "ConsoleHelper.h"
#include "VoyagerEngine.h"

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    if (!CreateNewConsole(1024))
        return -1;
#endif // _DEBUG

    std::cout << "Hello World!" << std::endl;

    VoyagerEngine voyager(500, 500, L"Voyager Game");
    int returnCode = WindowsApplication::Run(&voyager, hInstance, nCmdShow);

    return returnCode;
}

// TODOs:
// - V render a 3D spinning shape:
//      - V add depth testing
//      - V add root signature use for matrices etc
// - display fps:
//      - add text rendering
//      - add deltaTime measurement
// - add abstractions:
//      - scneObject
//      - camera
//      - buffer
//      - command list (with internal status tracking)
// - replace ComPtr with smart pointers
// - load a model:
//      - ?
// - add wireframe view
