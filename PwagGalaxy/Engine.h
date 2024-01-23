#pragma once

class Engine
{
public:
    Engine(UINT windowWidth, UINT windowHeight, std::wstring windowName);
    virtual ~Engine();

    virtual void OnInit(HWND windowHandle) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnKeyDown(UINT8 keyCode) {};
    virtual void OnKeyUp(UINT8 keyCode) {};
    virtual void OnMouseMove(int mouseX, int mouseY) {};
    virtual void OnGotFocus() {};
    virtual void OnLostFocus() {};

    UINT GetWidth() const           { return windowWidth; };
    UINT GetHeight() const          { return windowHeight; };
    std::wstring GetTitle() const   { return windowName; };

protected:
    UINT            windowWidth, windowHeight;
    float           aspectRatio;
    std::wstring    windowName;
    HWND            windowHandle;
};

