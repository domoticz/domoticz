#include "stdafx.h"


#if defined WIN32

#include <Windows.h>
#include "WindowsHelper.h"
//#include <fcntl.h>
//#include <io.h>
#include <cstdio>
#include <iostream>
//#include <fstream>
#include "resource.h"

extern bool g_bStopApplication;

console::console()
{
	m_old_cout=NULL;
	m_old_cerr=NULL;
	m_old_cin=NULL;
}

console::~console()
{
	if (m_old_cout)
	{
		// reset the standard streams
		std::cin.rdbuf(m_old_cin);
		std::cerr.rdbuf(m_old_cerr);
		std::cout.rdbuf(m_old_cout);

		// remove the console window
		FreeConsole();
	}
}

BOOL console::IsConsoleVisible()
{
	HWND hWnd = GetConsoleWindow();
	if (hWnd==NULL)
		return false;
	return IsWindowVisible(hWnd);
}

BOOL console::SetConsoleWindowSize(const int x, const int y)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	if (h == INVALID_HANDLE_VALUE)
		return FALSE;

	// If either dimension is greater than the largest console window we can have,
	// there is no point in attempting the change.
	{
		COORD largestSize = GetLargestConsoleWindowSize(h);
		if (x > largestSize.X)
			return FALSE;//The x dimension is too large
		if (y > largestSize.Y)
			return FALSE;//The y dimension is too large
	}

	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	if (!GetConsoleScreenBufferInfo(h, &bufferInfo))
		return FALSE;//Unable to retrieve screen buffer info

	SMALL_RECT& winInfo = bufferInfo.srWindow;
	COORD windowSize = { winInfo.Right - winInfo.Left + 1, winInfo.Bottom - winInfo.Top + 1 };

	if (windowSize.X > x || windowSize.Y > y)
	{
		// window size needs to be adjusted before the buffer size can be reduced.
		SMALL_RECT info =
		{
			0,
			0,
			x < windowSize.X ? x - 1 : windowSize.X - 1,
			y < windowSize.Y ? y - 1 : windowSize.Y - 1
		};

		if (!SetConsoleWindowInfo(h, TRUE, &info))
			return FALSE;//Unable to resize window before resizing buffer
	}

	COORD size = { x, y };
	if (!SetConsoleScreenBufferSize(h, size))
		return FALSE;//Unable to resize screen buffer.

	SMALL_RECT info = { 0, 0, x - 1, y - 1 };
	if (!SetConsoleWindowInfo(h, TRUE, &info))
		return FALSE;//Unable to resize window after resizing buffer.

	return TRUE;
}

void console::OpenHideConsole()
{
	HWND hWnd = GetConsoleWindow();
	if (hWnd!=NULL) {
		if (IsWindowVisible(hWnd))
			ShowWindow( hWnd, SW_HIDE );
		else
			ShowWindow( hWnd, SW_SHOW );
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
		return;
	}

	// create a console window
	AllocConsole();
//	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12); //display in red ;)

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);

	HWND hwnd = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu (hwnd, FALSE);
	//HINSTANCE hinstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE);
	while (DeleteMenu (hmenu, 0, MF_BYPOSITION))
		;
	// redirect std::cout to our console window
	m_old_cout = std::cout.rdbuf();
	m_out.open("CONOUT$");
	std::cout.rdbuf(m_out.rdbuf());

	// redirect std::cerr to our console window
	m_old_cerr = std::cerr.rdbuf();
	m_err.open("CONOUT$");
	std::cerr.rdbuf(m_err.rdbuf());

	// redirect std::cin to our console window
	m_old_cin = std::cin.rdbuf();
	m_in.open("CONIN$");
	std::cin.rdbuf(m_in.rdbuf());
	
	SetConsoleWindowSize(140, 30);

	SetConsoleTitle("Domoticz Home Automation System");
}

console myconsole;

void RedirectIOToConsole()
{
	myconsole.OpenHideConsole();
}

#define WM_TRAYICON				(WM_USER+58)
#define WM_TRAYEXIT				(WM_USER+59)
#define WM_SHOWCONSOLEWINDOW	(WM_USER+60)

const char g_szClassName[] = "DomoticzWindow";
HINSTANCE g_hInstance=NULL;
HANDLE g_hConsoleOut=NULL;
HWND g_hWnd;

#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000000
#endif

BOOL TrayMessage(DWORD dwMessage, const char *szInfo)
{
	NOTIFYICONDATA tnd;
	ZeroMemory(&tnd,sizeof(NOTIFYICONDATA));

	tnd.cbSize = NOTIFYICONDATAA_V2_SIZE;

	tnd.hWnd = g_hWnd;
	tnd.uID = 100;
	tnd.uCallbackMessage = WM_TRAYICON;
	tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP | NIF_SHOWTIP;
	if (dwMessage==NIM_ADD)
		tnd.uFlags|=NIF_SHOWTIP;
	tnd.hIcon = LoadIcon(g_hInstance,MAKEINTRESOURCE (IDR_MAINFRAME));
	strcpy(tnd.szTip, "Domoticz Home Automation");
	tnd.uTimeout=5;
	if (szInfo!=NULL)
	{
		tnd.uFlags|=NIF_INFO;
		strcpy(tnd.szInfo, szInfo);
		strcpy(tnd.szInfoTitle, tnd.szTip);
	}

	return Shell_NotifyIcon(dwMessage, &tnd);
}

void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
		int indexMenu=0;
		InsertMenu(hMenu, indexMenu++, MF_BYPOSITION, WM_SHOWCONSOLEWINDOW, "Show/Hide Console Window");
		CheckMenuItem(hMenu,WM_SHOWCONSOLEWINDOW,(myconsole.IsConsoleVisible())?MF_CHECKED:MF_UNCHECKED);

		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
		InsertMenu(hMenu, indexMenu++, MF_BYPOSITION, WM_TRAYEXIT, "E&xit");

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_TRAYICON:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			//RedirectIOToConsole();
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hwnd);
			break;
		} 
		break; 
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case WM_TRAYEXIT:
			g_bStopApplication = true;
			DestroyWindow(hwnd);
			break;
		case WM_SHOWCONSOLEWINDOW:
			RedirectIOToConsole();
			break;
		}
		break;
	case WM_CLOSE:
		g_bStopApplication = true;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		g_bStopApplication = true;
		TrayMessage(NIM_DELETE, NULL);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

bool InitWindowsHelper(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, const int iWebPort, const bool bStartWebBrowser)
{
	g_hInstance=hInstance;
	WNDCLASSEX wc;

	//Step 1: Registering the Window Class
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Step 2: Creating the Window
	g_hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Domoticz Home Automation",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 340, 20,
		NULL, NULL, hInstance, NULL);

	if(g_hWnd == NULL)
	{
		MessageBox(NULL,"Failed creating Domoticz Main Window!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	char szTrayInfo[100];
	sprintf(szTrayInfo,"Application Started.\nWebserver port: %d",iWebPort);
	TrayMessage(NIM_ADD,szTrayInfo);
	ShowWindow(g_hWnd, SW_HIDE);
#ifndef _DEBUG
	if (bStartWebBrowser)
	{
		char szURL[100];
		sprintf(szURL,"http://127.0.0.1:%d",iWebPort);
		ShellExecute(NULL, "open", szURL, NULL, NULL, SW_SHOWNORMAL);
	}
#endif
	return true;
}

void ShowSystemTrayNotification(const char *szMessage)
{
	TrayMessage(NIM_ADD,szMessage);
}

int	uname(struct utsname *putsname)
{
	strcpy(putsname->sysname,"Windows");
	strcpy(putsname->machine,"x86");
	strcpy(putsname->nodename,"Domoticz");
	strcpy(putsname->release,"1.2.3");
	strcpy(putsname->version,"latest");

	return 0;
}

#endif