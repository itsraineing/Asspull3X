#if !WIN32
#error Sorry, Windows only.
#endif

#include "asspull.h"
#include "SDL_syswm.h"
#include "ini.h"

#include <Windows.h>
#include <commdlg.h>

HWND hWnd;
HINSTANCE hInstance;
int uiData, uiCommand;

HWND hWndAbout = NULL;
BOOL CALLBACK AboutWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_COMMAND:
		//EndDialog(hwndDlg, wParam);
		DestroyWindow(hwndDlg);
		hWndAbout = NULL;
		return TRUE;
	}
	return FALSE;
}

void WndProc(void* userdata, void* hWnd, unsigned int message, Uint64 wParam, Sint64 lParam)
{
	if (message == WM_COMMAND)
	{
		SDL_Log("WM_COMMAND %d %d", wParam, lParam);
		if (wParam > 1000 && wParam < 2000)
		{
			uiCommand = (int)(wParam - 1000);
			if (uiCommand == cmdAbout)
			{
				uiCommand = cmdNone;
				//DialogBox(hInstance, MAKEINTRESOURCE(102), (HWND)hWnd, (DLGPROC)AboutWndProc);
				if (!IsWindow(hWndAbout))
				{
					hWndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(102), (HWND)hWnd, (DLGPROC)AboutWndProc);
					ShowWindow(hWndAbout, SW_SHOW);
				}
			}
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

		HMENU menuBar = LoadMenu(hInstance, MAKEINTRESOURCE(101));

		MENUINFO mainInfo =
		{
			sizeof(MENUINFO),
			MIM_APPLYTOSUBMENUS | MIM_STYLE,
			MNS_MODELESS | MNS_AUTODISMISS,
			0,
			NULL,
			NULL,
			0
		};
		SetMenuInfo(menuBar, &mainInfo);

		SetMenu(hWnd, menuBar);
	}
	return 0;
}

bool ShowFileDlg(bool toSave, char* target, size_t max, const char* filter)
{
	OPENFILENAME ofn;
	wchar_t szFile[260];
	wchar_t sFilter[512];
	mbstowcs_s(NULL, szFile, 260, target, max);
	mbstowcs_s(NULL, sFilter, 512, filter, 256);
	wchar_t* f = sFilter;
	while (*f)
	{
		if (*f == '|')
			*f = 0;
		f++;
	}
	*f++ = 0;
	*f++ = 0;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = sFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if ((!toSave && (GetOpenFileName(&ofn) == TRUE)) || (toSave && (GetSaveFileName(&ofn) == TRUE)))
	{
		wcstombs_s(NULL, target, max, ofn.lpstrFile, max);
		return true;
	}
	return false;
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

