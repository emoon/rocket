#include <SDL.h>
#include <SDL_syswm.h>
#include <emgui/Emgui.h>
#include <emgui/GFXBackend.h>
#include <stdio.h>
#include "Editor.h"
#include "Menu.h"
#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif

#define REPEAT_DELAY 250
#define REPEAT_RATE 50
static SDL_SysWMinfo window_info;
static SDL_Window* window;
static struct {  // for key repeat emulation
    SDL_Keysym keysym;
    Uint32 time;
    Uint32 repeattime;
} last_keydown;

void swapBuffers() {
    SDL_GL_SwapWindow(window);
}

void Window_populateRecentList(text_t** files) {
    int i;
    for (i = 0; i < 4; i++)
        if (files[i][0] != 0)
            printf("recent %d: %s\n", i + 1, files[i]);
}

void Window_setTitle(const text_t* title) {
    SDL_SetWindowTitle(window, title);
}

int getFilename(text_t* path, int save) {
#ifdef HAVE_GTK
    GtkWidget* d = gtk_file_chooser_dialog_new(
        save ? "Save File" : "Open File", NULL, save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL, save ? "Save" : "Open", GTK_RESPONSE_ACCEPT, NULL);
    gint res;
    if (save)
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
    res = gtk_dialog_run(GTK_DIALOG(d));
    if (res == GTK_RESPONSE_ACCEPT) {
        char* buf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        strncpy(path, buf, 512);
        path[511] = '\0';
        g_free(buf);
    }
    gtk_widget_destroy(d);
    while (gtk_events_pending())
        gtk_main_iteration();
    return !!(res == GTK_RESPONSE_ACCEPT);
#else
    printf(save ? "Save to: " : "Load from: ");
    fgets(path, 512, stdin);
    if (path[0] == 0) {
        printf("Aborted\n");
        return 0;
    } else {
        if (path[0] != 0 && path[strlen(path) - 1] == '\n')
            path[strlen(path) - 1] = 0;  // cut newline
        return 1;
    }
#endif
}

int Dialog_open(text_t* path) {
    return getFilename(path, 0);
}

int Dialog_save(text_t* path) {
    return getFilename(path, 1);
}

void Dialog_showColorPicker(unsigned int* color) {
#ifdef HAVE_GTK
    GtkWidget* d = gtk_color_selection_dialog_new("Select Color");
    GtkColorSelection* sel =
        GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(d)));
    GdkColor c = {
        0,
    };
    gint res;
    c.red = (*color & 0xFF) * 0x101;
    c.green = ((*color >> 8) & 0xFF) * 0x101;
    c.blue = ((*color >> 16) & 0xFF) * 0x101;
    gtk_color_selection_set_current_color(sel, &c);
    res = gtk_dialog_run(GTK_DIALOG(d));
    if ((res == GTK_RESPONSE_ACCEPT) || (res == GTK_RESPONSE_OK)) {
        gtk_color_selection_get_current_color(sel, &c);
        *color = 0xFF000000u | (c.red >> 8) | (c.green & 0x00FF00u) | ((c.blue << 8) & 0xFF0000u);
    }
    gtk_widget_destroy(d);
    while (gtk_events_pending())
        gtk_main_iteration();
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
            *color = 0xFF000000u | ((raw & 0xF) * 0x110000u) | (((raw >> 4) & 0xF) * 0x001100u) |
                     (((raw >> 8) & 0xF) * 0x000011u);
            break;
        case 6:
            *color = 0xFF000000u | (raw & 0x00FF00u) | ((raw >> 16) & 0x0000FFu) | ((raw << 16) & 0xFF0000u);
            break;
        default:
            printf("Invalid color value, ignoring.\n");
            break;
    }
#endif
}

void Dialog_showError(const text_t* text) {
#ifdef HAVE_GTK
    GtkWidget* d = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", text);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    while (gtk_events_pending())
        gtk_main_iteration();
#else
    printf("Error: %s\n", text);
#endif
}

int mapSdlEmKeycode(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT:
            return EMGUI_KEY_ARROW_LEFT;
        case SDLK_RIGHT:
            return EMGUI_KEY_ARROW_RIGHT;
        case SDLK_UP:
            return EMGUI_KEY_ARROW_UP;
        case SDLK_DOWN:
            return EMGUI_KEY_ARROW_DOWN;
        case SDLK_KP_ENTER:
            return EMGUI_KEY_ENTER;
        case SDLK_RETURN:
            return EMGUI_KEY_ENTER;
        case SDLK_ESCAPE:
            return EMGUI_KEY_ESC;
        case SDLK_TAB:
            return EMGUI_KEY_TAB;
        case SDLK_BACKSPACE:
            return EMGUI_KEY_BACKSPACE;
        case SDLK_SPACE:
            return EMGUI_KEY_SPACE;
        case SDLK_PAGEUP:
            return EMGUI_KEY_PAGE_UP;
        case SDLK_PAGEDOWN:
            return EMGUI_KEY_PAGE_DOWN;
        default:
            return key >= ' ' && key <= 127 ? key : 0;
    }
}

static int modifiers;

int getModifiers() {
    return modifiers;
}

void updateModifier(int mask, int flag) {
    modifiers = (modifiers & ~mask) | (flag ? mask : 0);
}

int updateModifiers(SDL_Keymod mod) {
    updateModifier(EMGUI_KEY_SHIFT, !!(mod & (KMOD_LSHIFT | KMOD_RSHIFT)));
    updateModifier(EMGUI_KEY_CTRL, !!(mod & (KMOD_LCTRL | KMOD_RCTRL)));
    updateModifier(EMGUI_KEY_ALT, !!(mod & (KMOD_LALT | KMOD_RALT)));
    return getModifiers();
}

int updateModifierPress(SDL_Keycode key, int state) {
    switch (key) {
        case SDLK_LSHIFT:
            updateModifier(EMGUI_KEY_SHIFT, state);
            return 0;
        case SDLK_RSHIFT:
            updateModifier(EMGUI_KEY_SHIFT, state);
            return 0;
        case SDLK_LCTRL:
            updateModifier(EMGUI_KEY_CTRL, state);
            return 0;
        case SDLK_RCTRL:
            updateModifier(EMGUI_KEY_CTRL, state);
            return 0;
        case SDLK_LALT:
            updateModifier(EMGUI_KEY_ALT, state);
            return 0;
        case SDLK_RALT:
            updateModifier(EMGUI_KEY_ALT, state);
            return 0;
        default:
            return 1;
    }
}

int checkMenu(int key, int mod, const MenuDescriptor* item) {
    for (; item->name; item++) {
        if (key == item->key && mod == item->winMod) {
            // printf("Menuevent %d %s\n", item->id, item->name);
            if (key == EMGUI_KEY_TAB || key == EMGUI_KEY_ENTER || key == EMGUI_KEY_BACKSPACE) {
                Emgui_sendKeyinput(key, getModifiers());
            }
            Editor_menuEvent(item->id);
            return 0;
        }
    }
    return 1;
}

int checkRecent(int key, int mod) {
    if (mod == EMGUI_KEY_CTRL && key >= '1' && key <= '4') {
        // printf("Load recent %d\n", key - '1');
        Editor_menuEvent(EDITOR_MENU_RECENT_FILE_0 + key - '1');
        return 0;
    }
    return 1;
}

int sendMenu(int key, int mod) {
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

void sendKey(int key, int modifiers) {
    Emgui_sendKeyinput(key, modifiers);
    Editor_keyDown(key, -1, modifiers);
}

// sdl keycodes used only internally, mapped to emgui items in the very beginning
void updateKey(SDL_Keysym* sym) {
    int keycode = mapSdlEmKeycode(sym->sym);
    int modifiers = updateModifiers(sym->mod);

    if (!keycode) {
        if (updateModifierPress(sym->sym, 1)) {
            // printf("bad key\n");
        }
    } else {
        if (sendMenu(keycode, modifiers))
            sendKey(keycode, modifiers);
    }
}

int handleKeyDown(SDL_KeyboardEvent* ev) {
    last_keydown.keysym = ev->keysym;
    last_keydown.time = last_keydown.repeattime = SDL_GetTicks();
    updateKey(&ev->keysym);
    return 0;
}

void handleKeyUp(SDL_KeyboardEvent* ev) {
    if (ev->keysym.sym == last_keydown.keysym.sym) {
        last_keydown.keysym.sym = SDLK_UNKNOWN;
    }
    updateModifierPress(ev->keysym.sym, 0);
}

void handleMouseMotion(SDL_MouseMotionEvent* ev) {
    Emgui_setMouseLmb(!!(ev->state & SDL_BUTTON_LMASK));
    Emgui_setMousePos(ev->x, ev->y);
}

void handleMouseButton(SDL_MouseButtonEvent* ev) {
    switch (ev->button) {
        case SDL_BUTTON_LEFT:
            Emgui_setMouseLmb(ev->state == SDL_PRESSED);
            break;
        default:
            break;
    }
}

void handleMouseWheel(SDL_MouseWheelEvent* ev) {
    Editor_scroll(-ev->x, -ev->y, getModifiers());
}

void resize(int w, int h) {
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowFullscreen(window, 0);
    EMGFXBackend_updateViewPort(w, h);
    Editor_setWindowSize(w, h);
}

int handleEvent(SDL_Event* ev) {
    switch (ev->type) {
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
        case SDL_MOUSEWHEEL:
            handleMouseWheel(&ev->wheel);
            break;
        case SDL_WINDOWEVENT:
            if (ev->window.windowID != SDL_GetWindowID(window))
                break;
            switch (ev->window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    resize(ev->window.data1, ev->window.data2);
                    break;
            }
        default:
            // printf("Unknown SDL event %d\n", ev->type);
            break;
    }
    return 0;
}

int doEvents() {
    SDL_Event ev;
    int quit = 0;

    while (SDL_PollEvent(&ev))  // SDL_WaitEvent()
        quit |= handleEvent(&ev);

    return quit;
}

void run(SDL_Window* window) {
    for (;;) {
        if (doEvents()) {
            break;
        }

        if (window_info.subsystem == SDL_SYSWM_WAYLAND) {
            if (last_keydown.keysym.sym != SDLK_UNKNOWN) {
                Uint32 time_now = SDL_GetTicks();
                if (time_now > last_keydown.time + REPEAT_DELAY) {
                    if (time_now > last_keydown.repeattime + REPEAT_RATE) {
                        last_keydown.repeattime = time_now;
                        updateKey(&last_keydown.keysym);
                    }
                }
            }
        }

        Editor_timedUpdate();
        Editor_update();
    }
}

// this just in working directory
#define RECENTS_FILE ".rocket-recents.txt"

void saveRecents() {
    int i;
    text_t** recents = Editor_getRecentFiles();

    FILE* fh = fopen(RECENTS_FILE, "w");
    if (!fh)
        return;

    for (i = 0; i < 4; i++)
        fprintf(fh, "%s\n", recents[i]);

    fclose(fh);
}

void loadRecents() {
    int i;
    text_t** recents = Editor_getRecentFiles();

    FILE* fh = fopen(RECENTS_FILE, "r");
    if (!fh)
        return;

    // printf("Recent files:\n");
    for (i = 0; i < 4; i++) {
        fgets(recents[i], 2048, fh);  // size looked up in Editor.c

        if (strlen(recents[i]) < 2 || recents[i][strlen(recents[i]) - 1] != '\n') {
            recents[i][0] = 0;
            break;  // eof or too long line
        }

        recents[i][strlen(recents[i]) - 1] = 0;  // strip newline
                                                 // printf("%d: %s\n", i, recents[i]);
    }

    // printf("total %d\n", i);
    for (; i < 4; i++)  // clear the rest if less than 4 recents
        recents[i][0] = 0;
    fclose(fh);
}

int main(int argc, char* argv[]) {
#ifdef HAVE_GTK
    gtk_init(&argc, &argv);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // SDL_EnableKeyRepeat(200, 20);
    window = SDL_CreateWindow("Rocket", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_VERSION(&window_info.version);
    SDL_GetWindowWMInfo(window, &window_info);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    loadRecents();

    EMGFXBackend_create();
    Editor_create();
    EMGFXBackend_updateViewPort(800, 600);
    Editor_setWindowSize(800, 600);

    run(window);

    saveRecents();

    SDL_Quit();
    return 0;
}
