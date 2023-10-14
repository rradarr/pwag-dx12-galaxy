#pragma once

#include "stdafx.h"

#include "Engine.h"

class WindowsApplication
{
public:
    static int Run(Engine* _gameEngine, HINSTANCE hInstance, int nCmdShow);

protected:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static HWND windowHandle;
};

