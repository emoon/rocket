#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>
#include "../Editor.h"
#include "resource.h"
#include "Menu.h"
#include "afxres.h"
#include <string.h>
#include <emgui/emgui.h>
#include <emgui/gfxbackend.h> 

void Window_buildMenu();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HWND s_window;
static HINSTANCE s_instance;
static HDC s_dc;
static HGLRC s_context;
static bool s_active;
static HKEY s_regConfigKey;
static HMENU s_recentFilesMenu;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Window_populateRecentList(text_t** files);
static bool setRegString(HKEY key, const wchar_t* name, const wchar_t* value, int size);
static bool getRegString(HKEY key, wchar_t* out, const wchar_t* name);
static void saveRecentFileList();
static void getRecentFiles();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void closeWindow()
{
	if (s_context)
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(s_context);
		s_context = 0;
	}

	if (s_window)
	{
		ReleaseDC(s_window, s_dc);
		DestroyWindow(s_window);
	}

	UnregisterClass(L"GLRocket", s_instance);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void swapBuffers()
{
	SwapBuffers(s_dc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
bool createWindow(const wchar_t* title, int width, int height)
{
	GLuint format;
	WNDCLASS wc;
	DWORD exStyle;
	DWORD style;
	RECT rect;

	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	rect.left = 0;
	rect.right = width;
	rect.top = 0;
	rect.bottom= height;

	EMGFXBackend_create();

	s_instance = GetModuleHandle(0);
	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc	= (WNDPROC)WndProc;
	wc.hInstance = s_instance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"RocketEditor";
	wc.hIcon = LoadIcon(s_instance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"Failed To Register Window Class", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
	
	exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	style = WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&rect, style, FALSE, exStyle);

	// Create The Window
	if (!(s_window = CreateWindowEx(exStyle, L"RocketEditor", title, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
							  0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, s_instance, NULL)))
	{
		closeWindow();								// Reset The Display
		return FALSE;								// Return FALSE
	}

	if (!(s_dc = GetDC(s_window)))
	{
		closeWindow();
		return FALSE;
	}

	if (!(format = ChoosePixelFormat(s_dc, &pfd)))
	{
		closeWindow();
		return FALSE;
	}

	if (!SetPixelFormat(s_dc, format,&pfd))
	{
		closeWindow();
		return FALSE;
	}

	if (!(s_context = wglCreateContext(s_dc)))
	{
		closeWindow();
		return FALSE;
	}

	if (!wglMakeCurrent(s_dc, s_context))
	{
		closeWindow();
		return FALSE;
	}

	if (RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\Gnu Rocket", &s_regConfigKey) != ERROR_SUCCESS)
	{
		if (ERROR_SUCCESS != RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Gnu Rocket", &s_regConfigKey))
		{
			// messagebox here
			return FALSE;
		}
	}

	Editor_create();

	getRecentFiles();
	Window_buildMenu();
	Window_populateRecentList(Editor_getRecentFiles());

	ShowWindow(s_window, SW_SHOW);
	SetForegroundWindow(s_window);
	SetFocus(s_window);

	return TRUE;
}

static ACCEL s_accelTable[512];
static int s_accelCount = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void formatName(wchar_t* outName, int keyMod, int key, const wchar_t* name)
{
	wchar_t modName[64] = L"";
	wchar_t keyName[64] = L""; 

	if ((keyMod & EMGUI_KEY_WIN))
		wcscat(modName, L"Win-");

	if ((keyMod & EMGUI_KEY_CTRL))
		wcscat(modName, L"Ctrl-");

	if ((keyMod & EMGUI_KEY_ALT))
		wcscat(modName, L"Alt-");

	if ((keyMod & EMGUI_KEY_SHIFT))
		wcscat(modName, L"Shift-");

	if (key < 127)
	{
		keyName[0] = (wchar_t)(char)(key & ~0x20);
		keyName[1] = 0;
	}
	else
	{
		switch (key)
		{
			case EMGUI_KEY_ARROW_DOWN : wcscpy_s(keyName, sizeof(keyName), L"Down"); break;
			case EMGUI_KEY_ARROW_UP: wcscpy_s(keyName, sizeof(keyName), L"Up"); break;
			case EMGUI_KEY_ARROW_RIGHT: wcscpy_s(keyName, sizeof(keyName), L"Right"); break;
			case EMGUI_KEY_ARROW_LEFT: wcscpy_s(keyName, sizeof(keyName), L"Left"); break;
			case EMGUI_KEY_ESC: wcscpy_s(keyName, sizeof(keyName), L"ESC"); break;
			case EMGUI_KEY_TAB: wcscpy_s(keyName, sizeof(keyName), L"TAB"); break;
			case EMGUI_KEY_BACKSPACE: wcscpy_s(keyName, sizeof(keyName), L"Delete"); break;
			case EMGUI_KEY_ENTER: wcscpy_s(keyName, sizeof(keyName), L"Enter"); break;
			case EMGUI_KEY_SPACE: wcscpy_s(keyName, sizeof(keyName), L"Space"); break;
			case EMGUI_KEY_PAGE_UP: wcscpy_s(keyName, sizeof(keyName), L"Page Up"); break;
			case EMGUI_KEY_PAGE_DOWN: wcscpy_s(keyName, sizeof(keyName), L"Page Down"); break;
		}
	}

	wsprintf(outName, L"%s\t%s%s", name, modName, keyName);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void addAccelarator(const MenuDescriptor* desc)
{
	uint8_t virt = 0;
	uint32_t winMod = desc->winMod;
	uint32_t key = desc->key;
	ACCEL* accel = &s_accelTable[s_accelCount++];

	if (winMod & EMGUI_KEY_ALT)
		virt |= 0x10;
	if (winMod & EMGUI_KEY_CTRL)
		virt |= 0x08;
	if (winMod & EMGUI_KEY_SHIFT)
		virt |= 0x04;

	if (key < 127)
	{
		if (virt != 0)
		{
			accel->key = (char)(key & ~0x20);
			virt |= 1;
		}
		else
		{
			accel->key = (char)(key);
		}
	}
	else
	{
		virt |= 1;

		switch (key)
		{
			case EMGUI_KEY_ARROW_DOWN : accel->key = VK_DOWN; break; 
			case EMGUI_KEY_ARROW_UP:  accel->key = VK_UP; break;
			case EMGUI_KEY_ARROW_RIGHT: accel->key = VK_RIGHT; break;
			case EMGUI_KEY_ARROW_LEFT: accel->key = VK_LEFT; break;
			case EMGUI_KEY_ESC: accel->key = VK_ESCAPE; break;
			case EMGUI_KEY_TAB: accel->key = VK_TAB; break;
			case EMGUI_KEY_BACKSPACE: accel->key = VK_BACK; break;
			case EMGUI_KEY_ENTER: accel->key = VK_RETURN; break;
			case EMGUI_KEY_SPACE: accel->key = VK_SPACE; break;
			case EMGUI_KEY_PAGE_DOWN: accel->key = VK_NEXT; break;
			case EMGUI_KEY_PAGE_UP: accel->key = VK_PRIOR; break;
		}
	}

	accel->fVirt = virt;
	accel->cmd = (WORD)desc->id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void buildSubMenu(HMENU parentMenu, MenuDescriptor menuDesc[], wchar_t* name)
{
	MenuDescriptor* desc = &menuDesc[0];
	HMENU menu = CreatePopupMenu();
    AppendMenu(parentMenu, MF_STRING | MF_POPUP, (UINT)menu, name);

	while (desc->name)
	{
		if (desc->id == EDITOR_MENU_SEPARATOR)
		{
			AppendMenu(menu, MF_SEPARATOR, 0, L"-");
		}
		else if (desc->id == EDITOR_MENU_SUB_MENU)
		{
			HMENU subMenu = CreatePopupMenu();
    		AppendMenu(menu, MF_STRING | MF_POPUP, (UINT)subMenu, desc->name);

    		// A bit hacky but will save us time searching for it later on

    		if (!wcscmp(desc->name, L"Recent Files"))
				s_recentFilesMenu = subMenu;
		}
		else
		{
			wchar_t temp[256];
			formatName(temp, desc->winMod, desc->key, desc->name);
			AppendMenu(menu, MF_STRING, desc->id, temp);
			addAccelarator(desc);
		}

		desc++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_buildMenu()
{
	int i = 0;
	HMENU mainMenu = CreateMenu();
    buildSubMenu(mainMenu, g_fileMenu, L"&File");
    buildSubMenu(mainMenu, g_editMenu, L"&Edit");
    buildSubMenu(mainMenu, g_viewMenu, L"&View");
	SetMenu(s_window, mainMenu);

	// Add accelerators for the recents files 

	for (i = 0; i < 4; ++i)
	{
		ACCEL* accel = &s_accelTable[s_accelCount++];
		accel->fVirt = 0x08 | 0x1;
		accel->key = (WORD)('1' + i);
		accel->cmd = (WORD)(EDITOR_MENU_RECENT_FILE_0 + i);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_setTitle(const wchar_t* title)
{
	SetWindowText(s_window, title);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_populateRecentList(text_t** files)
{
	int i;

	while (0 != RemoveMenu(s_recentFilesMenu, 0, MF_BYPOSITION));

	for (i = 0; i < 4; ++i)
	{
		wchar_t name[2048];
	
		if (!wcscmp(files[i], L""))
			continue;
		
		wsprintf(name, L"%s\tCtrl-&%d", files[i], i + 1);
		AppendMenu(s_recentFilesMenu, MF_STRING, EDITOR_MENU_RECENT_FILE_0 + i, name); 
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int onKeyDown(WPARAM wParam, LPARAM lParam)
{
	int key = -1;

	int foo = (int)(lParam >> 16); 
	(void)foo;

    switch (wParam)
    {
        case VK_LEFT : key = EMGUI_KEY_ARROW_LEFT; break;
        case VK_UP : key = EMGUI_KEY_ARROW_UP; break;
        case VK_RIGHT : key = EMGUI_KEY_ARROW_RIGHT; break;
        case VK_DOWN : key = EMGUI_KEY_ARROW_DOWN; break;
        case VK_SPACE : key = ' '; break;

        default:
        {
			int p, t = (int)wParam;
			(void)t;
            wParam = MapVirtualKey((UINT)wParam, 2) & 0x0000ffff;
			p = wParam;
			(void)p;
			wParam = (WPARAM) CharUpperA((LPSTR)wParam);

            if((wParam >=  32 && wParam <= 126) ||
               (wParam >= 160 && wParam <= 255) )
            {
                return (int)wParam;
            }
        }
    }
	
	return key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getModifiers()
{
	int modifiers = 0;

	modifiers |= GetKeyState(VK_SHIFT) < 0 ? EMGUI_KEY_SHIFT : 0;
	modifiers |= GetKeyState(VK_CONTROL) < 0 ? EMGUI_KEY_CTRL : 0;
	modifiers |= GetKeyState(VK_MENU) < 0 ? EMGUI_KEY_ALT : 0;
	modifiers |= GetKeyState(VK_LWIN) < 0 ? EMGUI_KEY_COMMAND : 0;
	modifiers |= GetKeyState(VK_RWIN) < 0 ? EMGUI_KEY_COMMAND : 0;

	return modifiers; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_ACTIVATE:
		{
			if (!HIWORD(wParam))
				s_active = true;
			else
				s_active = true;

			break;
		}

		case WM_LBUTTONDOWN:
		{
			Emgui_setMouseLmb(1);
			Editor_update();
			break;
		}

		case WM_LBUTTONUP:
		{
			Emgui_setMouseLmb(0);
			Editor_update();
			break;
		}

		case WM_MOUSEWHEEL:
		{
			float delta = -((float)GET_WHEEL_DELTA_WPARAM(wParam) / 20);
			Editor_scroll(0.0f, delta, getModifiers());
			break;
		}

		case WM_MOUSEMOVE:
		{
			const short pos_x = GET_X_LPARAM(lParam); 
			const short pos_y = GET_Y_LPARAM(lParam);

			if (wParam & MK_LBUTTON)
				Emgui_setMouseLmb(1);
			else
				Emgui_setMouseLmb(0);

			Emgui_setMousePos(pos_x, pos_y);
			Editor_update();
			return 0;
		}

        case WM_KEYDOWN:
        //case WM_SYSKEYDOWN:
		{
			int key = onKeyDown(wParam, lParam);

			if (key != -1)
			{
				int modifier = getModifiers();
				Emgui_sendKeyinput(key, modifier);
				Editor_keyDown(key, -1, modifier);
				Editor_update();
			}

			break;
		}

		case WM_COMMAND:
		{
			// make sure we send tabs, enters and backspaces down to emgui as well, a bit hacky but well

			if (LOWORD(wParam) == EDITOR_MENU_TAB)
				Emgui_sendKeyinput(EMGUI_KEY_TAB, getModifiers());

			if (LOWORD(wParam) == EDITOR_MENU_ENTER_CURRENT_V)
				Emgui_sendKeyinput(EMGUI_KEY_ENTER, getModifiers());

			if (LOWORD(wParam) == EDITOR_MENU_DELETE_KEY)
				Emgui_sendKeyinput(EMGUI_KEY_BACKSPACE, getModifiers());

			switch (LOWORD(wParam))
			{
				case EDITOR_MENU_OPEN:
				case EDITOR_MENU_SAVE:
				case EDITOR_MENU_SAVE_AS:
				case EDITOR_MENU_REMOTE_EXPORT:
				case EDITOR_MENU_RECENT_FILE_0:
				case EDITOR_MENU_RECENT_FILE_1:
				case EDITOR_MENU_RECENT_FILE_2:
				case EDITOR_MENU_RECENT_FILE_3:
				case EDITOR_MENU_UNDO:
				case EDITOR_MENU_REDO:
				case EDITOR_MENU_CANCEL_EDIT:
				case EDITOR_MENU_DELETE_KEY:
				case EDITOR_MENU_CUT:
				case EDITOR_MENU_COPY:
				case EDITOR_MENU_PASTE:
				case EDITOR_MENU_SELECT_TRACK:
				case EDITOR_MENU_BIAS_P_001:
				case EDITOR_MENU_BIAS_P_01:
				case EDITOR_MENU_BIAS_P_1:
				case EDITOR_MENU_BIAS_P_10:
				case EDITOR_MENU_BIAS_P_100:
				case EDITOR_MENU_BIAS_P_1000:
				case EDITOR_MENU_BIAS_N_001:
				case EDITOR_MENU_BIAS_N_01:
				case EDITOR_MENU_BIAS_N_1:
				case EDITOR_MENU_BIAS_N_10:
				case EDITOR_MENU_BIAS_N_100:
				case EDITOR_MENU_BIAS_N_1000:
				case EDITOR_MENU_INTERPOLATION:
				case EDITOR_MENU_ENTER_CURRENT_V:
				case EDITOR_MENU_TAB:
				case EDITOR_MENU_PLAY:
				case EDITOR_MENU_ROWS_UP:
				case EDITOR_MENU_ROWS_DOWN:
				case EDITOR_MENU_PREV_BOOKMARK:
				case EDITOR_MENU_NEXT_BOOKMARK:
				case EDITOR_MENU_FIRST_TRACK:
				case EDITOR_MENU_LAST_TRACK:
				case EDITOR_MENU_PREV_KEY:
				case EDITOR_MENU_NEXT_KEY:
				case EDITOR_MENU_FOLD_TRACK:
				case EDITOR_MENU_UNFOLD_TRACK:
				case EDITOR_MENU_FOLD_GROUP:
				case EDITOR_MENU_UNFOLD_GROUP:
				case EDITOR_MENU_TOGGLE_BOOKMARK:
				case EDITOR_MENU_CLEAR_BOOKMARKS:
				{
					Editor_menuEvent(LOWORD(wParam));
					break;
				}
			}
			break;
		}

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				// prevent screensaver and power saving
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				return 0;
			}
			break;
		}

		case WM_CLOSE:
		{
			int res;
			
			if (!Editor_needsSave())
			{
				PostQuitMessage(0);
				return 0;
			}

			res = MessageBox(window, L"Do you want to save the work?", L"Save before exit?", MB_YESNOCANCEL | MB_ICONQUESTION); 

			if (res == IDCANCEL)
				return 0;

			if (res == IDYES)
				Editor_saveBeforeExit();

			PostQuitMessage(0);
			return 0;
		}

		case WM_SIZE:
		{
			EMGFXBackend_updateViewPort(LOWORD(lParam), HIWORD(lParam));
			Editor_setWindowSize(LOWORD(lParam), HIWORD(lParam));
			Editor_update();
			return 0;
		}
	}

	return DefWindowProc(window, message, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CALLBACK timedCallback(HWND hwnd, UINT id, UINT_PTR ptr, DWORD meh)
{
	Editor_timedUpdate();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmndLine, int show)
{
	MSG	msg;
	HACCEL accel;
	bool done = false;	

	memset(&msg, 0, sizeof(MSG));

	if (!createWindow(L"RocketEditor" EDITOR_VERSION, 800, 600))
		return 0;

	accel = CreateAcceleratorTable(s_accelTable, s_accelCount);

	// Update timed function every 16 ms

	SetTimer(s_window, 1, 16, timedCallback);

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(s_window, accel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (WM_QUIT == msg.message) 
					done = true;
			}
		}

		Sleep(1); // to prevent hammering the thread
	}

	saveRecentFileList();

	closeWindow();
	return msg.wParam;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool setRegString(HKEY key, const wchar_t* name, const wchar_t* value, int size)
{
	return ERROR_SUCCESS == RegSetValueExW(key, name, 0, REG_SZ, (BYTE*)value, (size + 1) * 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool getRegString(HKEY key, wchar_t* out, const wchar_t* name)
{
	DWORD size = 0;
	DWORD type = 0;
	DWORD ret;

	if (ERROR_SUCCESS != RegQueryValueExW(key, name, 0, &type, (LPBYTE)NULL, &size)) 
		return false;
	
	if (REG_SZ != type) 
		return false;

	ret = RegQueryValueExW(key, name, 0, &type, (LPBYTE)out, &size);

	return ret == ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void getRecentFiles()
{
	int i = 0;
	text_t** recent_list = Editor_getRecentFiles();

	for (i = 0; i < 4; ++i)
	{
		wchar_t entryName[64];
		wsprintf(entryName, L"RecentFile%d", i); 
		getRegString(s_regConfigKey, recent_list[i], entryName); 		
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void saveRecentFileList()
{
	int i = 0;
	text_t** recent_list = Editor_getRecentFiles();

	for (i = 0; i < 4; ++i)
	{
		wchar_t entryName[64];
		wsprintf(entryName, L"RecentFile%d", i); 
		setRegString(s_regConfigKey, entryName, recent_list[i], wcslen(recent_list[i]));
	}
}


