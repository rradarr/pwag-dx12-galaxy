#pragma once

class Engine;

class WindowsApplication
{
public:
    static int Run(Engine* _gameEngine, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return windowHandle;  }

protected:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static HWND windowHandle;
    static HCURSOR cursor;
};

