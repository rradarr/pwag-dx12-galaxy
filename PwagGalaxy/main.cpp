#include "stdafx.h"

#include "ConsoleHelper.h"
#include "VoyagerEngine.h"
#include "WindowsApplication.h"

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
// V = done
// X = postponed / rejected
// ~ = in progress
// - V render a 3D spinning shape:
//      - V add depth testing
//      - V add root signature use for matrices etc
// - V display fps:
//      - X add text rendering:
//          - V add texture loading:
//              - V load a texture
//              - V fix descriptor heaps
//              - V fix black half of pyramid
//              - V display a model with a texture
//      - V add deltaTime measurement
// - V load a model:
// - ~ add wireframe view:
//      - create a second PSO for wireframe drawing
//      - detect a specific button press
//      - switch the PSOs with the button
// - add abstractions:
//      - scneObject:
//          V mesh
//          - material
//          - ? (properties such as translation and stuff?)
//      - camera
//      V buffer
//      - command list (with internal status tracking)
//      - resources (add resources, based on them allocate heap of good size and create resource views with good offsets. store offsets)
// - replace ComPtr with smart pointers


/*
Plan for resources:
- ResourceManager, loads the resources and creates them using abstractions Mesh, Texture, Material. Stores them in
arrays(?). Separate resources are coupled together in SceneObjects, that hold references to the resources.
*/