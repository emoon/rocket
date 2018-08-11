#include "SDL.h"
#include <stdio.h>
#include <emgui/Emgui.h>
#include <emgui/GFXBackend.h> 
#include "Editor.h"
#include "Menu.h"
#ifdef HAVE_GTK
	#include <gtk/gtk.h>
#endif

static SDL_Surface *screen;

void swapBuffers()
{
	SDL_GL_SwapBuffers();
}

void Window_populateRecentList(text_t** files)
{
	int i;
	for (i = 0; i < 4; i++)
		if (files[i][0] != 0)
			printf("recent %d: %s\n", i + 1, files[i]);
}

void Window_setTitle(const text_t *title)
{
	SDL_WM_SetCaption(title, NULL);
}

int getFilename(text_t *path, int save)
{
#ifdef HAVE_GTK
	GtkWidget* d = gtk_file_chooser_dialog_new(
		save ? "Save File" : "Open File",
		NULL,
		save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
		"Cancel", GTK_RESPONSE_CANCEL,
		save ? "Save" : "Open", GTK_RESPONSE_ACCEPT,
		NULL);
	gint res;
	if (save) gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
	res = gtk_dialog_run(GTK_DIALOG(d));
	if (res == GTK_RESPONSE_ACCEPT) {
		char* buf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
		strncpy(path, buf, 512);
		path[511] = '\0';
		g_free(buf);
	}
	gtk_widget_destroy(d);
	while (gtk_events_pending()) gtk_main_iteration();
	return !!(res == GTK_RESPONSE_ACCEPT);
#else
	printf(save ? "Save to: " : "Load from: ");
	fgets(path, 512, stdin);
	if (path[0] == 0)
	{
		printf("Aborted\n");
		return 0;
	}
	else
	{
		if (path[0] != 0 && path[strlen(path) - 1] == '\n')
			path[strlen(path) - 1] = 0; // cut newline
		return 1;
	}
#endif
}

int Dialog_open(text_t *path)
{
	return getFilename(path, 0);
}

int Dialog_save(text_t *path)
{
	return getFilename(path, 1);
}

void Dialog_showColorPicker(unsigned int* color)
{
#ifdef HAVE_GTK
	GtkWidget* d = gtk_color_selection_dialog_new("Select Color");
	GtkColorSelection* sel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(d)));
	GdkColor c = { 0, };
	gint res;
	c.red   = ( *color        & 0xFF) * 0x101;
	c.green = ((*color >>  8) & 0xFF) * 0x101;
	c.blue  = ((*color >> 16) & 0xFF) * 0x101;
	gtk_color_selection_set_current_color(sel, &c);
	res = gtk_dialog_run(GTK_DIALOG(d));
	if ((res == GTK_RESPONSE_ACCEPT) || (res == GTK_RESPONSE_OK)) {
		gtk_color_selection_get_current_color(sel, &c);
		*color = 0xFF000000u
		       |  (c.red >> 8)
		       | ( c.green      & 0x00FF00u)
		       | ((c.blue << 8) & 0xFF0000u);
	}
	gtk_widget_destroy(d);
	while (gtk_events_pending()) gtk_main_iteration();
#else
	char buf[16], *start, *end;
	unsigned int raw;
	int len;
	printf("Enter color as HTML hex code: ");
	fgets(buf, 16, stdin);
	start = &buf[(*buf == '#') ? 1 : 0];
	raw = strtoul(start, &end, 16);
	len = (*end < 32) ? (int)((intptr_t)end - (intptr_t)start) : 0;
	switch (len) {
		case 3:
			*color = 0xFF000000u
			       | (( raw       & 0xF) * 0x110000u)
			       | (((raw >> 4) & 0xF) * 0x001100u)
			       | (((raw >> 8) & 0xF) * 0x000011u);
			break;
		case 6:
			*color = 0xFF000000u
			       | ( raw        & 0x00FF00u)
			       | ((raw >> 16) & 0x0000FFu)
			       | ((raw << 16) & 0xFF0000u);
			break;
		default:
			printf("Invalid color value, ignoring.\n");
			break;
	}
#endif
}

void Dialog_showError(const text_t* text)
{
#ifdef HAVE_GTK
	GtkWidget* d = gtk_message_dialog_new(
		NULL, 0,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		"%s", text);
	gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
	while (gtk_events_pending()) gtk_main_iteration();
#else
	printf("Error: %s\n", text);
#endif
}

int mapSdlEmKeycode(SDLKey key)
{
	switch (key)
	{
		case SDLK_LEFT: return EMGUI_KEY_ARROW_LEFT;
		case SDLK_RIGHT: return EMGUI_KEY_ARROW_RIGHT;
		case SDLK_UP: return EMGUI_KEY_ARROW_UP;
		case SDLK_DOWN: return EMGUI_KEY_ARROW_DOWN;
		case SDLK_KP_ENTER: return EMGUI_KEY_ENTER;
		case SDLK_RETURN: return EMGUI_KEY_ENTER;
		case SDLK_ESCAPE: return EMGUI_KEY_ESC;
		case SDLK_TAB: return EMGUI_KEY_TAB;
		case SDLK_BACKSPACE: return EMGUI_KEY_BACKSPACE;
		case SDLK_SPACE: return EMGUI_KEY_SPACE;
		case SDLK_PAGEUP: return EMGUI_KEY_PAGE_UP;
		case SDLK_PAGEDOWN: return EMGUI_KEY_PAGE_DOWN;
		default: return key >= ' ' && key <= 127 ? key : 0;
	}
}

static int modifiers;

int getModifiers()
{
	return modifiers;
}

void updateModifier(int mask, int flag)
{
	modifiers = (modifiers & ~mask) | (flag ? mask : 0);
}

int updateModifiers(SDLMod mod)
{
	updateModifier(EMGUI_KEY_SHIFT, !!(mod & (KMOD_LSHIFT | KMOD_RSHIFT)));
	updateModifier(EMGUI_KEY_CTRL, !!(mod & (KMOD_LCTRL | KMOD_RCTRL)));
	updateModifier(EMGUI_KEY_ALT, !!(mod & (KMOD_LALT | KMOD_RALT)));
	return getModifiers();
}

int updateModifierPress(SDLKey key, int state)
{
	switch (key)
	{
		case SDLK_LSHIFT: updateModifier(EMGUI_KEY_SHIFT, state); return 0;
		case SDLK_RSHIFT: updateModifier(EMGUI_KEY_SHIFT, state); return 0;
		case SDLK_LCTRL: updateModifier(EMGUI_KEY_CTRL, state); return 0;
		case SDLK_RCTRL: updateModifier(EMGUI_KEY_CTRL, state); return 0;
		case SDLK_LALT: updateModifier(EMGUI_KEY_ALT, state); return 0;
		case SDLK_RALT: updateModifier(EMGUI_KEY_ALT, state); return 0;
		default: return 1;
	}
}

int checkMenu(int key, int mod, const MenuDescriptor *item)
{
	for (; item->name; item++)
	{
		if (key == item->key && mod == item->winMod)
		{
			//printf("Menuevent %d %s\n", item->id, item->name);
			if (key== EMGUI_KEY_TAB ||
					key== EMGUI_KEY_ENTER ||
					key== EMGUI_KEY_BACKSPACE)
			{
				Emgui_sendKeyinput(key, getModifiers());
			}
			Editor_menuEvent(item->id);
			return 0;
		}
	}
	return 1;
}

int checkRecent(int key, int mod)
{
	if (mod == EMGUI_KEY_CTRL && key >= '1' && key <= '4') {
		//printf("Load recent %d\n", key - '1');
		Editor_menuEvent(EDITOR_MENU_RECENT_FILE_0 + key - '1');
		return 0;
	}
	return 1;
}

int sendMenu(int key, int mod)
{
	if (!checkMenu(key, mod, g_fileMenu))
		return 0;
	if (!checkMenu(key, mod, g_editMenu))
		return 0;
	if (!checkMenu(key, mod, g_viewMenu))
		return 0;
	if (!checkRecent(key, mod))
		return 0;
	return 1;
}

void sendKey(int key, int modifiers)
{
	Emgui_sendKeyinput(key, modifiers);
	Editor_keyDown(key, -1, modifiers);
	Editor_update();
}

// sdl keycodes used only internally, mapped to emgui items in the very beginning
void updateKey(SDL_keysym *sym)
{
	int keycode = mapSdlEmKeycode(sym->sym);
	int modifiers = updateModifiers(sym->mod);

	if (!keycode)
	{
		if (updateModifierPress(sym->sym, 1)) {
			//printf("bad key\n");
		}
	}
	else
	{
		if (sendMenu(keycode, modifiers))
			sendKey(keycode, modifiers);
	}
}

int handleKeyDown(SDL_KeyboardEvent *ev)
{
	updateKey(&ev->keysym);
	return 0;
}

void handleKeyUp(SDL_KeyboardEvent *ev)
{
	updateModifierPress(ev->keysym.sym, 0);
}

void handleMouseMotion(SDL_MouseMotionEvent *ev)
{
	Emgui_setMouseLmb(!!(ev->state & SDL_BUTTON_LMASK));
	Emgui_setMousePos(ev->x, ev->y);
	Editor_update();
}

void handleMouseButton(SDL_MouseButtonEvent *ev)
{
	switch (ev->button)
	{
		case SDL_BUTTON_LEFT:
			Emgui_setMouseLmb(ev->state == SDL_PRESSED);
			Editor_update();
			break;
		case SDL_BUTTON_WHEELUP:
		case SDL_BUTTON_WHEELDOWN:
			if (ev->state == SDL_PRESSED)
			{
				float dir = ev->button == SDL_BUTTON_WHEELDOWN ? 1.0f : -1.0f;
				Editor_scroll(0.0f, dir, getModifiers());
				Editor_update();
			}
			break;
		default:
			break;
	}
}

void resize(int w, int h)
{
	screen = SDL_SetVideoMode(w, h, 32,
			SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_RESIZABLE);
	EMGFXBackend_updateViewPort(w, h);
	Editor_setWindowSize(w, h);
	Editor_update();
}

int handleEvent(SDL_Event *ev)
{
	switch (ev->type)
	{
		case SDL_QUIT:
			return 1;
		case SDL_KEYDOWN:
			return handleKeyDown(&ev->key);
		case SDL_KEYUP:
			handleKeyUp(&ev->key);
			break;
		case SDL_MOUSEMOTION:
			handleMouseMotion(&ev->motion);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			handleMouseButton(&ev->button);
			break;
		case SDL_ACTIVEEVENT:
		case SDL_VIDEOEXPOSE:
			Editor_update();
			break;
		case SDL_VIDEORESIZE:
			resize(ev->resize.w, ev->resize.h);
			break;
		default:
			//printf("Unknown SDL event %d\n", ev->type);
			break;
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
	for (;;)
	{
		if (doEvents())
			break;
		Editor_timedUpdate();
		SDL_Delay(16);
	}
}

// this just in working directory
#define RECENTS_FILE ".rocket-recents.txt"

void saveRecents()
{
	int i;
	text_t **recents = Editor_getRecentFiles();

	FILE *fh = fopen(RECENTS_FILE, "w");
	if (!fh)
		return;

	for (i = 0; i < 4; i++)
		fprintf(fh, "%s\n", recents[i]);

	fclose(fh);
}

void loadRecents()
{
	int i;
	text_t **recents = Editor_getRecentFiles();

	FILE *fh = fopen(RECENTS_FILE, "r");
	if (!fh)
		return;

	//printf("Recent files:\n");
	for (i = 0; i < 4; i++) {
		fgets(recents[i], 2048, fh); // size looked up in Editor.c

		if (strlen(recents[i]) < 2 ||
				recents[i][strlen(recents[i]) - 1] != '\n') {
			recents[i][0] = 0;
			break; // eof or too long line
		}

		recents[i][strlen(recents[i]) - 1] = 0; // strip newline
		//printf("%d: %s\n", i, recents[i]);
	}

	//printf("total %d\n", i);
	for (; i < 4; i++) // clear the rest if less than 4 recents
		recents[i][0] = 0;
	fclose(fh);
}

int main(int argc, char *argv[])
{
#ifdef HAVE_GTK
	gtk_init(&argc, &argv);
#endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "SDL_Init(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_EnableKeyRepeat(200, 20);
	screen = SDL_SetVideoMode(800, 600, 32,
			SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_RESIZABLE);

	if (!screen)
	{
		fprintf(stderr, "SDL_SetVideoMode(): %s\n", SDL_GetError());
		return 1;
	}

	loadRecents();

	EMGFXBackend_create();
	Editor_create();
	EMGFXBackend_updateViewPort(800, 600);
	Editor_setWindowSize(800, 600);
	Editor_update();

	run(screen);

	saveRecents();

	SDL_Quit();
	return 0;
}
