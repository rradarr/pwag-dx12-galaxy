#include "WindowsApplication.h"

int WindowsApplication::run(HINSTANCE hInstance, int nCmdShow)
{
	// register window class
	const wchar_t windowClassName[] = L"Voyager Game Window";

	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc		= WindowProc;
	windowClass.hInstance		= hInstance;
	windowClass.lpszClassName	= windowClassName;

	RegisterClass(&windowClass);

	// create the window
	HWND windowHanlde = CreateWindowEx(
		0,								// Optional window styles
		windowClassName,
		L"Voyager",						// Window text
		WS_OVERLAPPEDWINDOW,			// Window style
		CW_USEDEFAULT, CW_USEDEFAULT,	// Window size
		CW_USEDEFAULT, CW_USEDEFAULT,	// Window position
		NULL,							// Parent window    
		NULL,							// Menu
		hInstance,						// Instance handle
		NULL							// Additional application data
	);

	if (windowHanlde == NULL)
	{
		// raise some exception or smthn
	}

	ShowWindow(windowHanlde, nCmdShow);

	// additionally: init the game

	// run the message loop
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);	// this will call the WindowProc callback
	}

	// additionally: clean the game

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK WindowsApplication::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		// pass static_cast<UINT8>(wParam) to the game
		return 0;

	case WM_KEYUP:
		// pass static_cast<UINT8>(wParam) to the game
		return 0;

	case WM_PAINT:
		{
			//update and render the game

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// All painting occurs here, between BeginPaint and EndPaint.
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));

			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
