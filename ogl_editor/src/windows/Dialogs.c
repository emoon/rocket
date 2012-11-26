#include <windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_open(char* path, int pathSize)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = path;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = pathSize;
	ofn.lpstrFilter = "All\0*.*\0Rocket\0*.Rocket\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return GetOpenFileName(&ofn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_save(char* path, int pathSize)
{
	(void)path;
	(void)pathSize;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dialog_showColorPicker(unsigned int* color)
{
	(void)color;
}