#pragma once

#include "stdafx.h"

class WindowsApplication
{
public:
	int run(HINSTANCE hInstance, int nCmdShow);

protected:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

