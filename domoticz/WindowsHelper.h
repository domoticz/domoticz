#pragma once

#if defined WIN32

bool InitWindowsHelper(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, void *pQuitFunction, const int iWebPort);
void RedirectIOToConsole();

#endif