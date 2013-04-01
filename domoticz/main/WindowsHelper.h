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

bool InitWindowsHelper(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, void *pQuitFunction, const int iWebPort, const bool bStartWebBrowser);
void ShowSystemTrayNotification(const char *szMessage);
void RedirectIOToConsole();
BOOL TrayMessage(DWORD dwMessage, const char *szInfo);

//utsname
#define SYS_NMLN	32
struct utsname {
	char	sysname[SYS_NMLN];	/* Name of this OS. */
	char	nodename[SYS_NMLN];	/* Name of this network node. */
	char	release[SYS_NMLN];	/* Release level. */
	char	version[SYS_NMLN];	/* Version level. */
	char	machine[SYS_NMLN];	/* Hardware type. */
};
int	uname(struct utsname *);

#endif
