#if !WIN32
#error Sorry, Windows only.
#endif

#include "asspull.h"
#include "SDL_syswm.h"
#include "ini.h"

#include <Windows.h>

HWND hWnd;
HINSTANCE hInstance;
int uiData, uiCommand;

void WndProc(void* userdata, void* hWnd, unsigned int message, Uint64 wParam, Sint64 lParam)
{
	if (message == WM_COMMAND)
	{
		SDL_Log("WM_COMMAND %d %d", wParam, lParam);
		if (wParam > 1000 && wParam < 2000)
		{
			uiCommand = (int)(wParam - 1000);
		}
	}
}

int InitUI()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if(SDL_GetWindowWMInfo(sdlWindow, &info))
	{
		if (info.subsystem != SDL_SYSWM_WINDOWS)
			return -1;
		
		hWnd = info.info.win.window;
		hInstance = info.info.win.hinstance;

		SDL_SetWindowsMessageHook(WndProc, 0);

		HMENU menuBar = CreateMenu();
		
#define C2WP(x) (1000 + (x))

		HMENU mnuFile = CreatePopupMenu();
		AppendMenu(mnuFile, MF_STRING, C2WP(cmdLoadRom), _T("&Load ROM"));
		AppendMenu(mnuFile, MF_STRING, C2WP(cmdUnloadRom), _T("&Unload ROM"));
		AppendMenu(mnuFile, MF_STRING, C2WP(cmdReset), _T("&Reset"));
		AppendMenu(mnuFile, MF_SEPARATOR, 0, 0);
		AppendMenu(mnuFile, MF_STRING, C2WP(cmdQuit), _T("&Quit"));
		AppendMenu(menuBar, MF_POPUP, (UINT_PTR)mnuFile, _T("&File"));

		HMENU mnuDevices = CreatePopupMenu();
		AppendMenu(menuBar, MF_POPUP | MF_DISABLED, (UINT_PTR)mnuDevices, _T("&Devices"));

		HMENU mnuTools = CreatePopupMenu();
		AppendMenu(mnuTools, MF_STRING, C2WP(cmdScreenshot), _T("&Screenshot"));
		AppendMenu(mnuTools, MF_STRING, C2WP(cmdDump), _T("&Dump RAM"));
		AppendMenu(mnuTools, MF_STRING | MF_DISABLED, C2WP(cmdMemViewer), _T("&Memory viewer"));
		AppendMenu(mnuTools, MF_STRING | MF_DISABLED, C2WP(cmdAbout), _T("&About"));
		AppendMenu(menuBar, MF_POPUP, (UINT_PTR)mnuTools, _T("&Tools"));

		SetMenu(hWnd, menuBar);
	}
	return 0;
}

void SetStatus(char*)
{
}

extern unsigned char* pixels;

#define MAXSNOW 256
int snowData[MAXSNOW * 2];
int snowTimer = -1;

void LetItSnow()
{
	if (snowTimer == -1)
	{
		//Prepare
		srand(0xC001FACE);
		for (int i = 0; i < MAXSNOW; i++)
		{
			snowData[(i * 2) + 0] = rand() % 640;
			snowData[(i * 2) + 1] = rand() % 480;
		}
		snowTimer = 1;
	}

	snowTimer--;
	if (snowTimer == 0)
	{
		snowTimer = 4;
		for (int i = 0; i < MAXSNOW; i++)
		{
			snowData[(i * 2) + 0] += -1 + (rand() % 3);
			snowData[(i * 2) + 1] += rand() % 2;
			if (snowData[(i * 2) + 1] >= 480)
			{
				snowData[(i * 2) + 0] = rand() % 640;
				snowData[(i * 2) + 1] = -10;
			}
		}
	}

	for (int i = 0; i < MAXSNOW; i++)
	{
		int x = snowData[(i * 2) + 0];
		int y = snowData[(i * 2) + 1];
		if (x < 0 || y < 0 || x >= 640 || y >= 480)
			continue;
		auto target = ((y * 640) + x) * 4;
		pixels[target + 0] = pixels[target + 1] = pixels[target + 2] = 255;
	}
}

