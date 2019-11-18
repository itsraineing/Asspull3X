#if !WIN32
#error Sorry, Windows only.
#endif

#include "asspull.h"
#include "SDL_syswm.h"
#include <Windows.h>
#include <commdlg.h>

HWND hWnd;
HINSTANCE hInstance;
int uiData, uiCommand;

HWND hWndAbout = NULL, hWndMemViewer = NULL;

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

unsigned long memViewerOffset;

void MemViewerDraw(HWND hwndDlg)
{
}

void MemViewerComboProc(HWND hwndDlg)
{
	int index = SendDlgItemMessage(hwndDlg, 1000, CB_GETCURSEL, 0, 0);
	unsigned long areas[] =
	{
		BIOS_ADDR, CART_ADDR, WRAM_ADDR, DEVS_ADDR, REGS_ADDR, VRAM_ADDR
	};
	memViewerOffset = areas[index];
	char asText[64] = { 0 };
	sprintf_s(asText, 64, "%08X", memViewerOffset);
	SetDlgItemText(hwndDlg, 1002, asText);
	MemViewerDraw(hwndDlg);
}

BOOL CALLBACK MemViewerWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		hWndMemViewer = NULL;
		return TRUE;
	case WM_INITDIALOG:
	{
		HWND textBox = GetDlgItem(hwndDlg, 1002);
		SendDlgItemMessage(hwndDlg, 1002, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), FALSE);
		LPCSTR areas[] =
		{
			"BIOS", "Cart", "WRAM", "Devices", "Registers", "VRAM"
		};
		for (int i = 0; i < 6; i++)
			SendDlgItemMessage(hwndDlg, 1000, CB_ADDSTRING, 0, (LPARAM)areas[i]);
		SendDlgItemMessage(hwndDlg, 1000, CB_SETCURSEL, 1, 0);
		return true;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == 1000) //Combo
		{
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				MemViewerComboProc(hwndDlg);
				return true;
			}
		}
	}
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
				if (!IsWindow(hWndAbout))
				{
					hWndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(102), (HWND)hWnd, (DLGPROC)AboutWndProc);
					ShowWindow(hWndAbout, SW_SHOW);
				}
			}
			else if (uiCommand == cmdMemViewer)
			{
				uiCommand = cmdNone;
				if (!IsWindow(hWndMemViewer))
				{
					hWndMemViewer = CreateDialog(hInstance, MAKEINTRESOURCE(108), (HWND)hWnd, (DLGPROC)MemViewerWndProc);
					ShowWindow(hWndMemViewer, SW_SHOW);
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
	char sFilter[512];
	strcpy_s(sFilter, 512, filter);
	char* f = sFilter;
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
	ofn.lpstrFile = target;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = max;
	ofn.lpstrFilter = sFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	char cwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, cwd);

	if ((!toSave && (GetOpenFileName(&ofn) == TRUE)) || (toSave && (GetSaveFileName(&ofn) == TRUE)))
	{
		SetCurrentDirectory(cwd);
		strcpy_s(target, max, ofn.lpstrFile);
		return true;
	}

	SetCurrentDirectory(cwd);
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

