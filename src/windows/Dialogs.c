#include <windows.h>

extern HWND s_window;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_open(wchar_t* path, int pathSize) {
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = path;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = pathSize;
    ofn.lpstrFilter = L"All Files\0*.*\0Rocket\0*.Rocket\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileName(&ofn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_save(wchar_t* path) {
    OPENFILENAME dialog;
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"All Files (*.*)\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    dialog.lpstrDefExt = L"rocket";
    return GetSaveFileName(&dialog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dialog_showError(const wchar_t* text) {
    MessageBox(NULL, text, L"Error", MB_ICONERROR | MB_OK);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static COLORREF custColors[16];

void Dialog_showColorPicker(unsigned int* color) {
    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(CHOOSECOLOR));
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.lpCustColors = (LPDWORD)custColors;
    cc.rgbResult = *color & 0x00ffffff;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    cc.hwndOwner = s_window;
    if (ChooseColor(&cc))
        *color = cc.rgbResult | 0xff000000;
}
