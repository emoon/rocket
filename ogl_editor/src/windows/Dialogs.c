#include <windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_open(wchar_t* path, int pathSize)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = path;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = pathSize;
	ofn.lpstrFilter = L"All\0*.*\0Rocket\0*.Rocket\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return GetOpenFileName(&ofn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_save(wchar_t* path, int pathSize)
{
	OPENFILENAME dialog;
	(void)pathSize;
	ZeroMemory(&dialog, sizeof(dialog));
	dialog.lStructSize = sizeof(dialog);
	dialog.lpstrFilter = L"Rocket (*.rocket)\0*All Files (*.*)\0*.*\0";
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	dialog.lpstrDefExt = L"rocket";
	return GetSaveFileName(&dialog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dialog_showColorPicker(unsigned int* color)
{
	CHOOSECOLOR cc;
	ZeroMemory(&cc, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.rgbResult = *color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (ChooseColor(&cc)) 
		*color = cc.rgbResult;
}