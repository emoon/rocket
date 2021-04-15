#include <GLFW/glfw3.h>
#include <stdio.h>

#include "Editor.h"
#include "Menu.h"

void Window_setTitle(const text_t* title) {
	(void)title;
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

int Dialog_open(text_t* path) {
}

void Window_populateRecentList(text_t** files) {
}

void Dialog_showColorPicker(unsigned int* color) {
}

int Dialog_save(text_t* path) {
}

void swapBuffers() {
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[]) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
    	return 1;
    }

	GLFWwindow* window = glfwCreateWindow(800, 600, "Rocket Title", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

	glClearColor(1.0, 1.0, 0.0, 1.0);

    while (!glfwWindowShouldClose(window)) {
		int width = 0;
		int height = 0;

		glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


