//#include <emgui/Emgui.h>
//#include <emgui/GFXBackend.h>
#include <stdio.h>
#include "Editor.h"
#include "Menu.h"

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int get_filename(text_t* path, int save) {
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_open(text_t* path) {
    return get_filename(path, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_save(text_t* path) {
    return get_filename(path, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

/*
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
                              SDL_WINDOW_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_WINDOW_RESIZABLE);

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
*/
