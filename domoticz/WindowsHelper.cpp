#include "stdafx.h"

#if defined WIN32

#include <Windows.h>
#include "WindowsHelper.h"
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include "resource.h"

#define WM_TRAYICON				(WM_USER+58)
#define WM_TRAYEXIT				(WM_USER+59)

const char g_szClassName[] = "DomoticzWindow";
HINSTANCE g_hInstance=NULL;
const void *g_pQuitFunction=NULL;
HANDLE g_hConsoleOut=NULL;


void RedirectIOToConsole()
{
	int                        hConHandle;
	long                       lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE                       *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), 
		&coninfo);
	coninfo.dwSize.Y = 100;  // How many lines do you want to have in the console buffer
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), 
		coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );	
	SetConsoleTitle("Domoticz Home Automation System");   

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog 
	// point to console as well  Uncomment the next line if you are using c++ cio or comment if you don't
	std::ios::sync_with_stdio();
}


BOOL TrayMessage( HWND hWnd, DWORD dwMessage, const char *szInfo)
{
	char szTip[100];
	sprintf(szTip,"Domoticz Home Automation");
	
	NOTIFYICONDATA tnd;
	ZeroMemory(&tnd,sizeof(NOTIFYICONDATA));

	tnd.cbSize = NOTIFYICONDATAA_V2_SIZE;

	tnd.hWnd = hWnd;
	tnd.uID = 100;
	tnd.uCallbackMessage = WM_TRAYICON;
	tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP | NIF_SHOWTIP;
	if (dwMessage==NIM_ADD)
		tnd.uFlags|=NIF_SHOWTIP;
	tnd.hIcon = LoadIcon(g_hInstance,MAKEINTRESOURCE (IDR_MAINFRAME));
	strcpy(tnd.szTip, szTip);
	tnd.uTimeout=5;
	if (szInfo!=NULL)
	{
		tnd.uFlags|=NIF_INFO;
		strcpy(tnd.szInfo, szInfo);
		strcpy(tnd.szInfoTitle, szTip);
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
		InsertMenu(hMenu, -1, MF_BYPOSITION, WM_TRAYEXIT, "E&xit");

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
			DestroyWindow(hwnd);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		TrayMessage(hwnd,NIM_DELETE,NULL);
		if (g_pQuitFunction)
			((void (*)(void)) g_pQuitFunction)();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

bool InitWindowsHelper(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, void *pQuitFunction, const int iWebPort)
{
	g_pQuitFunction=pQuitFunction;
	g_hInstance=hInstance;
	WNDCLASSEX wc;
	HWND hwnd;

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
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Domoticz Home Automation",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 340, 20,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL,"Failed creating Domoticz Main Window!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	char szTrayInfo[100];
	sprintf(szTrayInfo,"Application Started.\nWebserver port: %d",iWebPort);
	TrayMessage(hwnd, NIM_ADD,szTrayInfo);
	ShowWindow(hwnd, SW_HIDE);
#ifndef _DEBUG
	char szURL[100];
	sprintf(szURL,"http://127.0.0.1:%d",iWebPort);
	ShellExecute(NULL, "open", szURL, NULL, NULL, SW_SHOWNORMAL);
#endif
	return true;
}


#endif