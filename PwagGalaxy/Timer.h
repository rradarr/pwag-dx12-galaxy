#pragma once

#include "stdafx.h"
#include <mutex>

class Timer
{
private:
    static Timer* instance;
    static std::mutex mutex;

protected:
    Timer();
    ~Timer() { delete instance; };

    double timerFrequency = 0.0;
    long long lastFrameTime = 0;
    double frameDelta = 0;
    float fps = 0.f;

public:
    Timer(Timer& other) = delete;
    void operator=(const Timer&) = delete;

    static Timer* GetInstance();

    // Call this once per frame
    void Update();

    float GetFps() { return fps; }
    double GetDeltaTime() { return frameDelta; }
};

