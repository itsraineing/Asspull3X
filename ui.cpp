#if !WIN32
#error Sorry, Windows only.
#endif

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "asspull.h"
#include "SDL_syswm.h"
#include "resource.h"
#include <Windows.h>
#include <commdlg.h>

HWND hWnd;
HINSTANCE hInstance;
int uiData, uiCommand;

HWND hWndAbout = NULL, hWndMemViewer = NULL, hWndOptions = NULL, hWndDevices = NULL;

BOOL CALLBACK AboutWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_COMMAND:
		//EndDialog(hwndDlg, wParam);
		DestroyWindow(hwndDlg);
		hWndAbout = NULL;
		return true;
	}
	return false;
}

unsigned long memViewerOffset;

void MemViewerDraw(HWND hwndDlg)
{
}

void MemViewerComboProc(HWND hwndDlg)
{
	int index = SendDlgItemMessage(hwndDlg, IDC_MEMVIEWERDROP, CB_GETCURSEL, 0, 0);
	uint32_t areas[] = { BIOS_ADDR, CART_ADDR, WRAM_ADDR, DEVS_ADDR, REGS_ADDR, VRAM_ADDR };
	memViewerOffset = areas[index];
	char asText[64] = { 0 };
	sprintf_s(asText, 64, "%08X", memViewerOffset);
	SetDlgItemText(hwndDlg, IDC_MEMVIEWEROFFSET, asText);
	MemViewerDraw(hwndDlg);
}

BOOL CALLBACK MemViewerWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			hWndMemViewer = NULL;
			return true;
		}
		case WM_INITDIALOG:
		{
			SendDlgItemMessage(hwndDlg, IDC_MEMVIEWEROFFSET, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), false);
			LPCSTR areas[] = { "BIOS", "Cart", "WRAM", "Devices", "Registers", "VRAM" };
			for (int i = 0; i < 6; i++)
				SendDlgItemMessage(hwndDlg, IDC_MEMVIEWERDROP, CB_ADDSTRING, 0, (LPARAM)areas[i]);
			SendDlgItemMessage(hwndDlg, IDC_MEMVIEWERDROP, CB_SETCURSEL, 1, 0);
			MemViewerComboProc(hwndDlg); //force update
			return true;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_MEMVIEWERDROP)
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					MemViewerComboProc(hwndDlg);
					return true;
				}
			}
		}
	}
	return false;
}

extern bool fpsCap, stretch200;

BOOL CALLBACK OptionsWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwndDlg);
		hWndOptions = NULL;
		return true;
	}
	case WM_INITDIALOG:
	{
		CheckDlgButton(hwndDlg, IDC_FPSCAP, fpsCap);
		CheckDlgButton(hwndDlg, IDC_ASPECT, stretch200);
		return true;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_FPSCAP)
		{
			fpsCap = (IsDlgButtonChecked(hwndDlg, IDC_FPSCAP) == 1);
			ini->Set("video", "fpscap", fpsCap ? "true" : "false");
			return true;
		}
		else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_ASPECT)
		{
			stretch200 = (IsDlgButtonChecked(hwndDlg, IDC_ASPECT) == 1);
			ini->Set("video", "stretch200", stretch200 ? "true" : "false");
			return true;
		}
		else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK)
		{
			DestroyWindow(hwndDlg);
			hWndOptions = NULL;
			return true;
		}
	}
	}
	return false;
}

void UpdateDevicePage(HWND hwndDlg)
{
	int selection = SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_GETCURSEL, 0, 0);
	auto device = devices[selection];

	//Hide everything regardless at first.
	int everything[] = { IDC_DEVNONE, IDC_DDFILE, IDC_DDINSERT, IDC_DDEJECT };
	for (int i = 0; i < 4; i++)
		ShowWindow(GetDlgItem(hwndDlg, everything[i]), SW_HIDE);

	if (device == NULL)
	{
		ShowWindow(GetDlgItem(hwndDlg, IDC_DEVNONE), SW_SHOW);
		SetDlgItemText(hwndDlg, IDC_HEADER, "No device");
	}
	else
	{
		switch (device->GetID())
		{
		case 0x0144:
			for (int i = 1; i < 4; i++)
				ShowWindow(GetDlgItem(hwndDlg, everything[i]), SW_SHOW);
			SetDlgItemText(hwndDlg, IDC_HEADER, "Disk drive");
			break;
		case 0x4C50:
			ShowWindow(GetDlgItem(hwndDlg, IDC_DEVNONE), SW_SHOW);
			SetDlgItemText(hwndDlg, IDC_HEADER, "Line printer");
			break;
		}
	}
}

void UpdateDeviceList(HWND hwndDlg)
{
	int selection = 0;
	if (SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_GETCOUNT, 0, 0))
		selection = SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_GETCURSEL, 0, 0);
	char item[64] = { 0 };
	SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < MAXDEVS; i++)
	{
		if (devices[i] == 0)
		{
			sprintf_s(item, 64, "%d. <None>", i + 1);
		}
		else
		{
			switch (devices[i]->GetID())
			{
			case 0x0144:
				sprintf_s(item, 64, "%d. Disk drive", i + 1);
				break;
			case 0x4C50:
				sprintf_s(item, 64, "%d. Line printer", i + 1);
				break;
			}
		}
		SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_ADDSTRING, 0, (LPARAM)item);
	}
	SendDlgItemMessage(hwndDlg, IDC_DEVLIST, LB_SETCURSEL, selection, 0);
	UpdateDevicePage(hwndDlg);
}

BOOL CALLBACK DevicesWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwndDlg);
		hWndDevices = NULL;
		return true;
	}
	case WM_INITDIALOG:
	{
		UpdateDeviceList(hwndDlg);
		return true;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_DEVLIST)
		{
			UpdateDevicePage(hwndDlg);
			return true;
		}
		else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK)
		{
			DestroyWindow(hwndDlg);
			hWndDevices = NULL;
			return true;
		}
	}
	}
	return false;
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
					hWndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUT), (HWND)hWnd, (DLGPROC)AboutWndProc);
					ShowWindow(hWndAbout, SW_SHOW);
				}
			}
			else if (uiCommand == cmdMemViewer)
			{
				uiCommand = cmdNone;
				if (!IsWindow(hWndMemViewer))
				{
					hWndMemViewer = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MEMVIEWER), (HWND)hWnd, (DLGPROC)MemViewerWndProc);
					ShowWindow(hWndMemViewer, SW_SHOW);
				}
			}
			else if (uiCommand == cmdOptions)
			{
				uiCommand = cmdNone;
				if (!IsWindow(hWndOptions))
				{
					hWndOptions = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_OPTIONS), (HWND)hWnd, (DLGPROC)OptionsWndProc);
					ShowWindow(hWndOptions, SW_SHOW);
				}
			}
			else if (uiCommand == cmdDevices)
			{
				uiCommand = cmdNone;
				if (!IsWindow(hWndDevices))
				{
					hWndDevices = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DEVICES), (HWND)hWnd, (DLGPROC)DevicesWndProc);
					ShowWindow(hWndDevices, SW_SHOW);
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

