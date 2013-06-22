#include "SDL.h"
#include <stdio.h>
#include <emgui/Emgui.h>

void swapBuffers()
{
	SDL_GL_SwapBuffers();
	printf("swap\n");
}

void Window_populateRecentList(text_t** files)
{
	int i;
	for (i = 0; i < 4; i++)
		if (wcscmp(files[i], "") != 0)
			wprintf(L"recent: %s\n", files[i]);
}

void Window_setTitle(const wchar_t *title)
{
	char mbyte[32];
	if (wcstombs(mbyte, title, sizeof(mbyte)) >= sizeof(mbyte))
		mbyte[sizeof(mbyte) - 1] = 0;
	SDL_WM_SetCaption(mbyte, NULL);
}

int Dialog_open(wchar_t* path, int pathSize)
{
	printf("dialog_open() not implemented\n");
	return 0;
#warning TODO retval
}

int Dialog_save(wchar_t* path)
{
	printf("dialog_save() not implemented\n");
	return 0;
#warning TODO retval
}

void Dialog_showColorPicker(unsigned int* color)
{
	*color = 0;
	printf("dialog_save() not implemented\n");
}

int handleKey(SDLKey key)
{
	switch (key) {
	case SDLK_ESCAPE:
		return 1;
	}
	return 0;
}

int handleEvent(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_QUIT:
		return 1;
	case SDL_KEYDOWN:
		return handleKey(ev->key.keysym.sym);
	}
	return 0;

}

int doEvents()
{
	SDL_Event ev;
	int quit = 0;

	while (SDL_PollEvent(&ev)) // SDL_WaitEvent()
		quit |= handleEvent(&ev);

	return quit;
}

void run(SDL_Surface *screen)
{
	for (;;) {
		if (doEvents())
			break;
		Editor_timedUpdate();
		printf("frame\n");
		SDL_Delay(160);
	}
}

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL_Init(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_OPENGL | SDL_GL_DOUBLEBUFFER);

	EMGFXBackend_create();
	Editor_create();
	// TODO: Editor_getRecentFiles()

	EMGFXBackend_updateViewPort(800, 600);
	Editor_setWindowSize(800, 600);
	Editor_update();
	run(screen);

	SDL_Quit();
	return 0;
}
