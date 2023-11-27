#include "Timer.h"

Timer* Timer::instance{ nullptr };
std::mutex Timer::mutex;

Timer::Timer()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);

    // seconds
    timerFrequency = double(li.QuadPart);

    // milliseconds
    //timerFrequency = double(li.QuadPart) / 1000.0;

    // microseconds
    //timerFrequency = double(li.QuadPart) / 1000000.0;

    QueryPerformanceCounter(&li);
    lastFrameTime = li.QuadPart;
}

Timer* Timer::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Timer();
    }
    return instance;
}

void Timer::Update()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);

    frameDelta = double(li.QuadPart - lastFrameTime) / timerFrequency;

    if (frameDelta > 0) {
        fps = 1.f / frameDelta;
    }

    lastFrameTime = li.QuadPart;
}
