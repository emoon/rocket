#pragma once

#include <emgui/Types.h>

struct RocketImage;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RocketGuiState
{
	int mousex;
	int mousey;
	int mouseDown;

	int hotItem;
	int activeItem;

	int kbdItem;
	int keyEntered;
	int keyMod;

	int lastWidget;

} RocketGuiState;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum RocketDrawType
{
	DRAWTYPE_NONE,
	DRAWTYPE_FILL,
	DRAWTYPE_IMAGE,
	DRAWTYPE_TEXT,
	DRAWTYPE_SLIDER,
};

enum RocketSliderDirection
{
	SLIDERDIRECTION_HORIZONTAL,
	SLIDERDIRECTION_VERTICAL,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RocketControlInfo
{
	enum RocketDrawType type;

	int x;
	int y;
	int width;
	int height;

	unsigned int color;
	struct RocketImage* imageData;
	char* text;

	// todo: Use union with all data instead
	int sliderThumbX;
	int sliderThumbY;
	int sliderThumbWidth;
	int sliderThumbHeight;

} RocketControlInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_init();
void RocketGui_reset();
void RocketGui_begin();
void RocketGui_end();

void RocketGui_beginVerticalStackPanelXY(int x, int y);
void RocketGui_beginHorizontalStackPanelXY(int x, int y);

void RocketGui_beginVerticalStackPanel();
void RocketGui_beginHorizontalStackPanel();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Different controlls and gui functions

RocketControlInfo* RocketGui_textLabel(const char* text);

void RocketGui_staticImage(const char* filename);
void RocketGui_fill(uint32_t color, int x, int y, int w, int h);
void RocketGui_drawBorder(uint32_t color0, uint32_t color1, int x, int y, int w, int h);
void RocketGui_textLabelXY(const char* text, int x, int y);

bool RocketGui_slider(int x, int y, int w, int h, int start, int end, 
					  enum RocketSliderDirection dir, int itemSpace, int* value);

bool RocketGui_buttonCoords(const char* text, int x, int y);
bool RocketGui_buttonCoordsImage(const char* text, int x, int y);

bool RocketGui_button(const char* text);
bool RocketGui_buttonImage(const char* filename);

