#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>
#include "../Editor.h"
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

	UnregisterClass("GLRocket", s_instance);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void swapBuffers()
{
	SwapBuffers(s_dc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
bool createWindow(const char* title, int width, int height)
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

	s_instance = GetModuleHandle(0);
	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc	= (WNDPROC)WndProc;
	wc.hInstance = s_instance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "RocketEditor";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, "Failed To Register Window Class", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
	
	exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	style = WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&rect, style, FALSE, exStyle);

	// Create The Window
	if (!(s_window = CreateWindowEx(exStyle, "RocketEditor", title, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
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

	EMGFXBackend_create();
	EMGFXBackend_updateViewPort(width, height);

	return TRUE;
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

			return 0;
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

		case WM_MOUSEMOVE:
		{
			const short pos_x = GET_X_LPARAM(lParam); 
			const short pos_y = GET_Y_LPARAM(lParam);
			Emgui_setMousePos(pos_x, pos_y);
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
	bool done = false;	

	memset(&msg, 0, sizeof(MSG));

	if (!createWindow("RocketEditor", 800, 600))
		return 0;

	EMGFXBackend_create();
	Editor_create();
	Editor_update();

	// Update timed function every 16 ms

	SetTimer(s_window, 1, 16, timedCallback);

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	closeWindow();
	return msg.wParam;
}
