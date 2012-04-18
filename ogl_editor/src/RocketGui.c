#include "RocketGui.h"
#include "MicroknightFont.h"
#include "GFXBackend.h"
#include <string.h>
#include <stdio.h>

#if defined(ROCKETGUI_MACOSX)
#include <sys/syslimits.h>
#else
// TODO: Include correct path
#define PATH_MAX 1024
#endif

///

enum
{
	MAX_CONTROLS = 1024,
	MAX_IMAGES = 1024,
};

RocketGuiState g_rocketGuiState;
uint32_t s_controlId;

const int s_fontWidth = 9;
const int s_fontHeight = 9;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum GuiPlacementState
{
	PLACEMENTSTATE_NONE,
	PLACEMENTSTATE_HORIZONAL,
	PLACEMENTSTATE_VERTICAL,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct GuiPlacementInfo
{
	enum GuiPlacementState state;
	int x;
	int y;

} GuiPlacementInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GuiPlacementInfo g_placementInfo;
RocketControlInfo g_controls[MAX_CONTROLS];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_beginHorizontalStackPanelXY(int x, int y)
{
	g_placementInfo.state = PLACEMENTSTATE_HORIZONAL;
	g_placementInfo.x = x;
	g_placementInfo.y = y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_beginVerticalStackPanelXY(int x, int y)
{
	g_placementInfo.state = PLACEMENTSTATE_VERTICAL;
	g_placementInfo.x = x;
	g_placementInfo.y = y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_begin()
{
	s_controlId = 1;
	g_controls[0].type = DRAWTYPE_NONE;
	memset(&g_placementInfo, 0, sizeof(GuiPlacementInfo));
}

/*
struct LoadedImages
{
	char filename[PATH_MAX][MAX_IMAGES];
	RocketImage image[MAX_IMAGES];
	uint32_t count;
} g_loadedImages;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool RocketGui_regionHit(const RocketControlInfo* control)
{
	if (g_rocketGuiState.mousex < control->x ||
		g_rocketGuiState.mousey < control->y ||
		g_rocketGuiState.mousex >= control->x + control->width ||
		g_rocketGuiState.mousey >= control->y + control->height)
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void initFont()
{
	uint32_t i;
	uint32_t* fontData;
	uint32_t* tempColorData = fontData = (uint32_t*)malloc(128 * 128 * sizeof(uint32_t));
	const uint8_t* data = s_microkinghtFontData; 

	// Build new texture

	const uint32_t fontColor = 0x7fffffff;

	for (i = 0; i < (128 * 128) / 8; ++i)
	{
		uint8_t color = *data++;
		*tempColorData++ = ((color >> 7) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 6) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 5) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 4) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 3) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 2) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 1) & 1) ? fontColor : 0;
		*tempColorData++ = ((color >> 0) & 1) ? fontColor : 0;
	}

	GFXBackend_setFont(fontData, 128, 128);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_init()
{
	initFont();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_reset()
{
	//memset(&g_loadedImages, 0, sizeof(struct LoadedImages)); // not really needed but good when debugging
	g_rocketGuiState.mousex = -1;
	g_rocketGuiState.mousey = -1; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static RocketImage* loadImage(const char* filename)
{
	uint32_t i;
	uint32_t index = g_loadedImages.count;

	HIPPO_ASSERT(filename);

	// TODO: Hash filenames?

	for (i = 0; i != index; ++i)
	{
		if (!strcmp(filename, g_loadedImages.filename[i]))
			return &g_loadedImages.image[i];
	}

	// Load new image

	if (!RocketImageLoader_loadFile(&g_loadedImages.image[index], filename))
	{
		printf("Unable to load %s\n", filename);
		return 0;
	}	
	
	strcpy(g_loadedImages.filename[index], filename); g_loadedImages.count++;

	return &g_loadedImages.image[index];
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void updatePlacement()
{


}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool RocketGui_regionHitSlider(const RocketControlInfo* control)
{
	if (g_rocketGuiState.mousex < control->sliderThumbX ||
		g_rocketGuiState.mousey < control->sliderThumbY ||
		g_rocketGuiState.mousex >= control->sliderThumbX + control->sliderThumbWidth ||
		g_rocketGuiState.mousey >= control->sliderThumbY + control->sliderThumbHeight)
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_drawBorder(uint32_t color0, uint32_t color1, int x, int y, int w, int h)
{
	RocketGui_fill(color0, x, y, 1, h);
	RocketGui_fill(color0, x, y, w, 1);
	RocketGui_fill(color1, x + w, y, 1, h);
	RocketGui_fill(color1, x, y + h, w, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_fill(uint32_t color, int x, int y, int w, int h)
{
	uint32_t controlId = 0;
	RocketControlInfo* control = 0; 

	// Setup the control
	controlId = s_controlId++;
	control = &g_controls[controlId];
	control->type = DRAWTYPE_FILL;
	control->x = x;
	control->y = y;
	control->width = w;
	control->height = h;
	control->color = color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_textLabelXY(const char* text, int x, int y)
{
	uint32_t controlId = 0;
	RocketControlInfo* control = 0; 

	// Setup the control
	controlId = s_controlId++;
	control = &g_controls[controlId];
	control->type = DRAWTYPE_TEXT;
	control->color = 0;
	control->x = x;
	control->y = y;
	control->text = (char*)text;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float floatClamp(float value, float low, float high)
{
	if (value < low)
		value = low;

	if (value > high)
		value = high;

	return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RocketGui_slider(int x, int y, int w, int h, int start, int end, enum RocketSliderDirection dir, int itemSpace, int* value)
{
	int thumbPosition = 0;
	uint32_t controlId = 0;
	float thumbSize = 0.0f, range;
	RocketControlInfo* control = 0; 

	// Setup the control
	controlId = s_controlId++;
	control = &g_controls[controlId];
	control->type = DRAWTYPE_SLIDER;
	control->x = x;
	control->y = y;
	control->width = w;
	control->height = h;

	// considering how much stuff we have have in the slider we need to calculate how much space the scolling area actually
	// is so we can resize the thumb 

	range = end - start;

	if (dir == SLIDERDIRECTION_VERTICAL)
	{
		float v = *value;
		int itemsHeight = (end - start) * itemSpace;
		int sliderHeigthArea = h - y;
		
		if (itemsHeight <= 0)
			itemsHeight = 1; 

		thumbPosition = y + ((v / range) * h);

		thumbSize = (float)sliderHeigthArea / (float)itemsHeight; 
		thumbSize = /*floatClamp(thumbSize, 0.05, 1.0f)*/ 1.0f * sliderHeigthArea;

		control->sliderThumbX = x + 1;
		control->sliderThumbY = thumbPosition + 1;
		control->sliderThumbWidth = w - 1;
		control->sliderThumbHeight = (int)thumbSize;
	}
	else
	{
		// handle Horizontal here:
	}

	if (RocketGui_regionHitSlider(control))
	{
		g_rocketGuiState.hotItem = controlId;
    	if (g_rocketGuiState.activeItem == 0 && g_rocketGuiState.mouseDown)
      		g_rocketGuiState.activeItem = controlId;
	}

	//
	

	if (g_rocketGuiState.activeItem == controlId)
	{
		if (dir == SLIDERDIRECTION_VERTICAL)
		{
			float mouseYrelative;
			int mousePos = g_rocketGuiState.mousey - (y + control->sliderThumbHeight / 2);
			int mouseYlimit = control->height - control->sliderThumbHeight;
			if (mousePos < 0) mousePos = 0;
			if (mousePos > mouseYlimit) mousePos = mouseYlimit; 

			mouseYrelative = (float)mousePos / (float)control->height;
			control->sliderThumbY = (int)(y + mouseYrelative * h);
			*value = start + (range * mouseYrelative);
		}
		else
		{


		}

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RocketControlInfo* RocketGui_textLabel(const char* text)
{
	uint32_t controlId = 0;
	RocketControlInfo* control = 0; 

	// Setup the control
	controlId = s_controlId++;
	control = &g_controls[controlId];
	control->type = DRAWTYPE_TEXT;
	control->x = g_placementInfo.x;
	control->y = g_placementInfo.y; 
	control->width = strlen(text) * 9; // fix me
	control->height = 9; // fixme 
	control->text = (char*)text;
	control->color = 0;

	switch (g_placementInfo.state)
	{
		case PLACEMENTSTATE_NONE :
		{
			break;
		}

		case PLACEMENTSTATE_HORIZONAL :
		{
			g_placementInfo.x += control->width;
			break;
		}

		case PLACEMENTSTATE_VERTICAL :
		{
			g_placementInfo.y += 9; // TODO: Use correct size from font
			break;
		}
	}

	return control;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t genericImageControl(const char* filename)
{
	uint32_t controlId = 0;
	RocketControlInfo* control = 0; 
	struct RocketImage* image = 0; //loadImage(filename);

	if (!image)
		return ~0;

	// Setup the control
	
	controlId = s_controlId++;
	control = &g_controls[controlId];
	control->type = DRAWTYPE_IMAGE;
	control->x = g_placementInfo.x;
	control->y = g_placementInfo.y;
	//control->width = image->width;
	//control->height = image->height;
	control->imageData = image; 

	updatePlacement();

	switch (g_placementInfo.state)
	{
		case PLACEMENTSTATE_NONE :
		{
			break;
		}

		case PLACEMENTSTATE_HORIZONAL :
		{
			//g_placementInfo.x += image->width;
			break;
		}

		case PLACEMENTSTATE_VERTICAL :
		{
			//g_placementInfo.y += image->height;
			break;
		}
	}

	return controlId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_staticImage(const char* filename)
{
	genericImageControl(filename);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void highlightControl(RocketControlInfo* control)
{
	RocketGui_fill(0xb0ffffff, control->x, control->y, control->width, control->height);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RocketGui_buttonImage(const char* filename)
{
	RocketControlInfo* control;
	uint32_t controlId = 0;
	
	controlId = genericImageControl(filename);

	if (controlId == ~0)
		return false;

	control = &g_controls[controlId];

	if (RocketGui_regionHit(control))
	{
		g_rocketGuiState.hotItem = controlId;
    	if (g_rocketGuiState.activeItem == 0 && g_rocketGuiState.mouseDown)
      		g_rocketGuiState.activeItem = controlId;

   		//highlightControl(control);
	}

	if (g_rocketGuiState.mouseDown == 0 && g_rocketGuiState.hotItem == controlId && g_rocketGuiState.activeItem == controlId)
		return true;

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RocketGui_button(const char* text)
{
	RocketControlInfo* control;
	uint32_t controlId = 0;

	control = RocketGui_textLabel(text);
	controlId = s_controlId - 1; 

	if (controlId == ~0)
		return false;

	control = &g_controls[controlId];

	if (RocketGui_regionHit(control))
	{
		g_rocketGuiState.hotItem = controlId;
    	if (g_rocketGuiState.activeItem == 0 && g_rocketGuiState.mouseDown)
      		g_rocketGuiState.activeItem = controlId;

   		//highlightControl(control);
	}

	if (g_rocketGuiState.mouseDown == 0 && g_rocketGuiState.hotItem == controlId && g_rocketGuiState.activeItem == controlId)
		return true;

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RocketGui_end()
{
	if (g_rocketGuiState.mouseDown == 0)
	{
		g_rocketGuiState.activeItem = 0;
	}
	else
	{
		if (g_rocketGuiState.activeItem == 0)
			g_rocketGuiState.activeItem = -1;
	}

	GFXBackend_drawControls(&g_controls, s_controlId);
}

