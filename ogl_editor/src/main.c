#include "Editor.h"
#include <GL/glfw.h>
#include <Emgui.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW\n" );
		return -1;
	}

	if (!glfwOpenWindow(800, 600, 0, 0, 0, 0, 0, 0, GLFW_WINDOW))
	{
		printf("Failed to open GLFW window\n" );
		return -1;
	}

	glfwSetWindowTitle("RocketEditor");

	Editor_create();

	do
	{
		int width, height;

		glfwGetWindowSize(&width, &height);
		EMGFXBackend_updateViewPort(width, height);
		Editor_setWindowSize(width, height);

		Editor_update();

		glfwSwapBuffers();
	} 
	while (glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED));

	glfwTerminate();

	return 0;
}

