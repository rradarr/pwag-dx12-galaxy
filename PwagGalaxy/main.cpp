#include "stdafx.h"
#include <iostream>

#include "WindowsApplication.h"
#include "VoyagerEngine.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	std::cout << "Hello World!" << std::endl;

    std::cout << "Hello World!" << std::endl;

    VoyagerEngine voyager(500, 500, L"Voyager Game");
    int returnCode = WindowsApplication::Run(&voyager, hInstance, nCmdShow);

    return returnCode;
}