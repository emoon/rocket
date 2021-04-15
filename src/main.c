#include <GLFW/glfw3.h>
#include <stdio.h>

#include "Editor.h"
#include "Menu.h"
#include "emgui/Emgui.h"
#include "emgui/GFXBackend.h"
#include <string.h>

static int s_modifiers = 0;

void Window_setTitle(const text_t* title) {
	(void)title;
}

void Window_populateRecentList(text_t** files) {
}

//void Dialog_showColorPicker(unsigned int* color) {
//}

void swapBuffers() {}

#if 0

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loadRecents() {
/*
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
*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if ((mods & GLFW_MOD_SHIFT) && key == GLFW_KEY_LEFT_SHIFT) {
		printf("left shift\n");
	}

	if ((mods & GLFW_MOD_SHIFT) && key == GLFW_KEY_RIGHT_SHIFT) {
		printf("right shift\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		Emgui_setMouseLmb(action == GLFW_PRESS);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Editor_scroll(-xoffset, -yoffset, s_modifiers);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void window_size_callback(GLFWwindow* window, int width, int height) {
	EMGFXBackend_updateViewPort(width, height);
	Editor_setWindowSize(width, height);
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[]) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
    	return 1;
    }

	GLFWwindow* window = glfwCreateWindow(800, 600, "Rocket", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}

    glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    loadRecents();

    EMGFXBackend_create();
    Editor_create();
    EMGFXBackend_updateViewPort(800, 600);
    Editor_setWindowSize(800, 600);

    while (!glfwWindowShouldClose(window)) {
        Editor_timedUpdate();
        Editor_update();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


