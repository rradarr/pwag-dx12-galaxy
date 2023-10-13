#include "stdafx.h"
#include <iostream>

#include "WindowsApplication.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	std::cout << "Hello World!" << std::endl;

	WindowsApplication app;
	int returnCode = app.run(hInstance, nCmdShow);

	return returnCode;
}