#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>
#include "../Editor.h"
#include "resource.h"
#include "afxres.h"
#include <string.h>
#include <Emgui.h>
#include <GFXBackend.h> 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static HWND s_window;
static HINSTANCE s_instance;
static HDC s_dc;
static HGLRC s_context;
static bool s_active;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);

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

	ShowWindow(s_window, SW_SHOW);
	SetForegroundWindow(s_window);
	SetFocus(s_window);
	Editor_create();

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_setTitle(const wchar_t* title)
{
	SetWindowText(s_window, title);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_populateRecentList(char** files)
{
	(void)files;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int onKeyDown(WPARAM wParam, LPARAM lParam)
{
	int key = -1;

	int foo = (int)(lParam >> 16); 
	(void)foo;

    switch (wParam)
    {
        case VK_LEFT : key = EMGUI_ARROW_LEFT; break;
        case VK_UP : key = EMGUI_ARROW_UP; break;
        case VK_RIGHT : key = EMGUI_ARROW_RIGHT; break;
        case VK_DOWN : key = EMGUI_ARROW_DOWN; break;
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
			switch (LOWORD(wParam))
			{
				case ID_FILE_OPEN:
				{
					//Editor_menuEvent(EDITOR_MENU_OPEN);
					Editor_update();
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

	if (!createWindow(L"RocketEditor", 800, 600))
		return 0;

	accel = LoadAccelerators(instance, MAKEINTRESOURCE(IDR_ACCELERATOR));

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
	}

	closeWindow();
	return msg.wParam;
}
