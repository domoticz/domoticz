#pragma once

#if defined WIN32

#include <fstream>

class console
{
public:
	console();
	~console();
	void OpenHideConsole();
	BOOL IsConsoleVisible();
private:
	std::ofstream m_out;
	std::ofstream m_err;
	std::ifstream m_in;

	std::streambuf* m_old_cout;
	std::streambuf* m_old_cerr;
	std::streambuf* m_old_cin;
};

bool InitWindowsHelper(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, void *pQuitFunction, const int iWebPort);
void RedirectIOToConsole();

#endif