
#ifndef EMGUI_H_
#define EMGUI_H_

#include "Types.h"
#include <emgui/FontLayout.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EmguiSliderDirection
{
	EMGUI_SLIDERDIR_HORIZONTAL,
	EMGUI_SLIDERDIR_VERTICAL,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EmguiSpecialKey
{
	EMGUI_KEY_ARROW_DOWN = 0x100,
	EMGUI_KEY_ARROW_UP,
	EMGUI_KEY_ARROW_RIGHT,
	EMGUI_KEY_ARROW_LEFT,

	EMGUI_KEY_ESC,
	EMGUI_KEY_TAB,
	EMGUI_KEY_BACKSPACE,
	EMGUI_KEY_ENTER,
	EMGUI_KEY_SPACE,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EmguiKeyModifier
{
	EMGUI_KEY_WIN = 1, // windows key on Windows
	EMGUI_KEY_COMMAND = 1, // Command key on Mac OS X
	EMGUI_KEY_ALT = 2,
	EMGUI_KEY_CTRL = 4,
	EMGUI_KEY_SHIFT = 8,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EmguiMemoryLocation
{
	EMGUI_LOCATION_MEMORY,
	EMGUI_LOCATION_FILE,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EMGUI_COLOR32(r, g, b, a) (unsigned int)((a << 24) | (b << 16) | (g << 8) | r)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE uint32_t Emgui_color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (a << 24) | (b << 16) | (g << 8) | r;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE uint32_t Emgui_color32_getR(uint32_t color)
{
	return color & 0xff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE uint32_t Emgui_color32_getG(uint32_t color)
{
	return (color >> 8) & 0xff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE uint32_t Emgui_color32_getB(uint32_t color)
{
	return (color >> 16) & 0xff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Creation and state changes the application needs to call

bool Emgui_create();
void Emgui_destroy();
void Emgui_setMousePos(int posX, int posY);
void Emgui_setMouseLmb(int state);

void Emgui_begin();
void Emgui_end();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_beginVerticalPanelXY(int x, int y);
void Emgui_beginHorizontalPanelXY(int x, int y);

void Emgui_beginVerticalPanel();
void Emgui_beginHorizontalPanel();

void Emgui_setLayer(int layer);
void Emgui_setScissor(int x, int y, int w, int h);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Font functions

void Emgui_setFont(uint32_t fontId);
void Emgui_setDefaultFont();
bool Emgui_setFontByName(const char* ttfFontname);

int Emgui_loadFontTTF(const char* ttfFontname, float fontHeight);
int Emgui_loadFontBitmap(const char* buffer, int len, enum EmguiMemoryLocation location, 
						 int rangeStart, int rangeEnd, EmguiFontLayout* layout);
uint32_t Emgui_getTextSize(const char* text);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EditBox controlls

void Emgui_editBoxXY(int x, int y, int width, int height, int bufferLength, char* buffer);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Different controlls and gui functions

void Emgui_sendKeyinput(int keyCode, int modifier);
void Emgui_textLabel(const char* text);
void Emgui_drawLine(uint32_t color, int x0, int y0, int x1, int y1);
void Emgui_drawText(const char* text, int x, int y, uint32_t color);
void Emgui_drawTextFlipped(const char* text, int x, int y, uint32_t color);

void Emgui_staticImage(const char* filename);
void Emgui_fill(uint32_t color, int x, int y, int w, int h);
void Emgui_fillGrad(uint32_t color0, uint32_t color1, int x, int y, int w, int h);
void Emgui_drawBorder(uint32_t color0, uint32_t color1, int x, int y, int w, int h);
void Emgui_drawDots(uint32_t color, int* coords, int count);
void Emgui_textLabelXY(const char* text, int x, int y);

bool Emgui_slider(int x, int y, int w, int h, int start, int end, int largeVal,
				  enum EmguiSliderDirection dir, int itemSpace, int* value);

bool Emgui_buttonCoords(const char* text, uint32_t color, int x, int y, int width, int height);
bool Emgui_buttonCoordsImage(const char* text, int x, int y);

bool Emgui_button(const char* text);
bool Emgui_buttonImage(const char* filename);

void Emgui_setFirstControlFocus();
bool Emgui_hasKeyboardFocus();

void Emgui_radioButtonImage(void* image0, int size0, void* image1, int size1, enum EmguiMemoryLocation location, 
							uint32_t color, int x, int y, bool* stateIn);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions

static EMGUI_INLINE int emini(int a, int b)
{
	if (a < b)
		return a;

	return b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE int emaxi(int a, int b)
{
	if (a > b)
		return a;

	return b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EMGUI_INLINE int eclampi(int v, int low, int high)
{
	return emini(high, emaxi(v, low));
}

#endif // EMGUI_H_

