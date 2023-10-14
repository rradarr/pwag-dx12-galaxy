#pragma once

#include "stdafx.h"

bool RedirectConsoleIO();
bool ReleaseConsole();
void AdjustConsoleBuffer(int16_t minLength);
bool CreateNewConsole(int16_t minLength);