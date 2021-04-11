#include "Emgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Emgui_internal.h"
#include "External/stb_image.h"
#include "External/stb_truetype.h"
#include "GFXBackend.h"
#include "MicroknightFont.h"
#include "memory/LinearAllocator.h"

#if defined(emguiGUI_MACOSX)
#include <sys/syslimits.h>
#else
#define PATH_MAX 1024
#endif

struct EmguiImage;

typedef struct MouseState {
    int x;
    int y;
    int down;
} MouseState;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct EmguiState {
    MouseState mouse;

    int hotItem;
    int activeItem;

    int kbdItem;
    int keyCode;
    int keyMod;

    int lastWidget;

} EmguiState;

struct EmguiImageCache {
    uint64_t hash;
    uint64_t handle;
    int32_t width;
    int32_t height;
};

enum {
    MAX_IMAGES = 1024,
    EM_FONTBITMAP_WIDTH = 512,
    EM_FONTBITMAP_HEIGHT = 512,
};

static LinearAllocator s_allocator;
static LinearAllocatorRewindPoint s_rewindPoint;
static int g_loadedFontsCount = 1;
LoadedFont g_loadedFonts[MAX_FONTS];

EmguiState g_emguiGuiState;
uint32_t g_controlId;
uint32_t g_currentFont = 1;
uint32_t g_temp;
uint32_t g_defaultFont;
static int s_activeLayer = 0;

struct RenderData s_renderData;

static struct EmguiImageCache s_imageCache[MAX_IMAGES];
static int s_imageCacheCount;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum GuiPlacementState {
    PLACEMENTSTATE_NONE,
    PLACEMENTSTATE_HORIZONAL,
    PLACEMENTSTATE_VERTICAL,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct GuiPlacementInfo {
    enum GuiPlacementState state;
    int x;
    int y;

} GuiPlacementInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GuiPlacementInfo g_placementInfo;
EmguiControlInfo g_controls[MAX_CONTROLS];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static EMGUI_INLINE EmguiControlInfo* newControl()
{
    EmguiControlInfo* control = &g_controls[g_controlId];
    g_controlId++;
    return control;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_beginHorizontalStackPanelXY(int x, int y) {
    g_placementInfo.state = PLACEMENTSTATE_HORIZONAL;
    g_placementInfo.x = x;
    g_placementInfo.y = y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_beginVerticalStackPanelXY(int x, int y) {
    g_placementInfo.state = PLACEMENTSTATE_VERTICAL;
    g_placementInfo.x = x;
    g_placementInfo.y = y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// taken from:
// http://blade.nagaokaut.ac.jp/cgi-bin/scat.rb/ruby/ruby-talk/142054
//
// djb  :: 99.8601 percent coverage (60 collisions out of 42884)
// elf  :: 99.5430 percent coverage (196 collisions out of 42884)
// sdbm :: 100.0000 percent coverage (0 collisions out of 42884) (this is the algo used)
// ...

static uint32_t quickHash(const char* string) {
    uint32_t c;
    uint32_t hash = 0;

    const uint8_t* str = (const uint8_t*)string;

    while ((c = *str++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash & 0x7FFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint64_t loadImage(int* width, int* height, void* image, int length, enum EmguiMemoryLocation location) {
    int w = 0, h = 0, c = 0;
    void* imageData = 0;
    int i, imageCount = s_imageCacheCount;
    uint64_t imageId = (uint64_t)~0;
    uint64_t hash = (uintptr_t)image;

    if (location == EMGUI_LOCATION_FILE)
        hash = quickHash((const char*)image);

    // See if we have the image loaded already

    for (i = 0; i < imageCount; ++i) {
        if (s_imageCache[i].hash == hash) {
            *width = s_imageCache[i].width;
            *height = s_imageCache[i].height;
            return s_imageCache[i].handle;
        }
    }

    if (location == EMGUI_LOCATION_FILE) {
        imageData = stbi_load(image, &w, &h, &c, 0);

        if (!imageData)
            printf("EMGUI: Unable to load image %s\n", (char*)image);
    } else if (location == EMGUI_LOCATION_MEMORY) {
        imageData = stbi_load_from_memory(image, length, &w, &h, &c, 0);

        if (!imageData)
            printf("EMGUI: Unable to load image (from memory %p)\n", image);
    }

    if (imageData)
        imageId = EMGFXBackend_createTexture(imageData, w, h, c);

    s_imageCacheCount++;
    s_imageCache[imageCount].hash = hash;
    s_imageCache[imageCount].handle = imageId;
    s_imageCache[imageCount].width = w;
    s_imageCache[imageCount].height = h;

    *width = s_imageCache[i].width;
    *height = s_imageCache[i].height;

    return imageId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void createDefaultFont() {
    struct LoadedFont* loadedFont = &g_loadedFonts[0];
    uint8_t* tempColorData;
    uint8_t* colorData = tempColorData = (uint8_t*)malloc(128 * 128);
    const uint8_t* data = s_microkinghtFontData;
    uint32_t i;

    // Build new texture

    for (i = 0; i < (128 * 128) / 8; ++i) {
        uint8_t color = *data++;
        // font data is packed as 1 bit per pixel
        *tempColorData++ = ((color >> 7) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 6) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 5) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 4) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 3) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 2) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 1) & 1) ? 0xff : 0;
        *tempColorData++ = ((color >> 0) & 1) ? 0xff : 0;
    }

    loadedFont->handle = EMGFXBackend_createFontTexture(colorData, 128, 128);
    strcpy(loadedFont->name, "Microknight");
    loadedFont->altLookup = (unsigned short*)&s_microknightLayout[0].x;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void createStipplePattern() {
    static unsigned char pattern[32 * 34];
    static unsigned char tempData[32 * 34] = {0};
    int x, y, i, startX = 8, width = 16, t = 0;

    for (y = 0; y < 32; ++y) {
        unsigned char* line = &tempData[y * 32];

        for (i = 0; i < width; ++i)
            line[startX + i] = 0x1;

        /*
        for (int i = 0; i < width; ++i)
        {
            int p = startX + i;

            if (p <= -1)
                p = 32 + p;

            line[p] = 0x1;
        }
        startX--;
        */
    }

    // pack data to bits

    for (x = 0; x < (32 * 32) / 8; ++x) {
        unsigned char p = 0;

        p = tempData[t + 0] << 0;
        p |= tempData[t + 1] << 1;
        p |= tempData[t + 2] << 2;
        p |= tempData[t + 3] << 3;
        p |= tempData[t + 4] << 4;
        p |= tempData[t + 5] << 5;
        p |= tempData[t + 6] << 6;
        p |= tempData[t + 7] << 7;

        pattern[x] = p;
        t += 8;
    }

    EMGFXBackend_setStippleMask(pattern);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Emgui_loadFontBitmap(const char* buffer, int size, enum EmguiMemoryLocation location, int rangeStart, int rangeEnd,
                         EmguiFontLayout* layout)

{
    struct LoadedFont* loadedFont;
    void* imageData = 0;
    int fontId = 0, width = 0, height = 0, comp = 0;

    // Load the texture in to memory

    if (location == EMGUI_LOCATION_FILE)
        imageData = stbi_load(buffer, &width, &height, &comp, 0);
    else if (location == EMGUI_LOCATION_MEMORY)
        imageData = stbi_load_from_memory((void*)buffer, size, &width, &height, &comp, 0);

    // make sure we got the image data and that its an 8-bit (alpha only) texture

    if (!imageData || comp != 1) {
        free(imageData);
        return -1;
    }

    fontId = g_loadedFontsCount++;

    loadedFont = &g_loadedFonts[fontId];
    memset(loadedFont, 0, sizeof(LoadedFont));

    loadedFont->width = (uint16_t)width;
    loadedFont->height = (uint16_t)height;
    loadedFont->rangeStart = rangeStart;
    loadedFont->rangeEnd = rangeEnd;
    loadedFont->handle = EMGFXBackend_createFontTexture(imageData, width, height);
    loadedFont->layout = layout;

    free(imageData);

    return fontId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Track both y and x size and return a mask of them

uint32_t Emgui_getTextSize(const char* text) {
    uint32_t x_size = 0;
    int y_size = 0;
    int range_start;
    int range_end;
    int c;
    const LoadedFont* font = &g_loadedFonts[g_currentFont];

    if (!font->layout)
        return (8 << 16) | (int)(strlen(text) * 8);

    range_start = font->rangeStart;
    range_end = font->rangeEnd;
    c = (unsigned char)*text++;

    while (c != 0) {
        if (c >= range_start && c < range_end) {
            const int offset = c - range_start;
            x_size += font->layout[offset].xadvance;
            if (font->layout[offset].height > y_size)
                y_size = font->layout[offset].height;
        }

        c = *text++;
    }

    return (y_size << 16) | x_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Emgui_create() {
    const uint32_t size = 1024 * 1024;
    LinearAllocator_create(&s_allocator, malloc(size), size);
    createDefaultFont();
    createStipplePattern();
    g_emguiGuiState.kbdItem = -1;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_begin() {
    g_controlId = 1;
    g_controls[0].type = EMGUI_DRAWTYPE_NONE;

    memset(&s_renderData, 0, sizeof(s_renderData));

    memset(&g_placementInfo, 0, sizeof(GuiPlacementInfo));
    s_rewindPoint = LinearAllocator_getRewindPoint(&s_allocator);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool Emgui_regionHit(const EmguiControlInfo* control) {
    const int mouse_x = g_emguiGuiState.mouse.x;
    const int mouse_y = g_emguiGuiState.mouse.y;

    // need to figure out why adding 4 makes this better

    const int control_x = control->x + 4;
    const int control_y = control->y + 4;

    /*
    printf("%d %d - %d %d %d %d\n",
            mouse_x, mouse_y,
            control_x, control_y,
            control_x + control->width,
            control_y + control->height);
            */

    if (mouse_x < control_x || mouse_y < control_y || mouse_x >= control_x + control->width ||
        mouse_y >= control_y + control->height) {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_reset() {
    g_emguiGuiState.mouse.x = -1;
    g_emguiGuiState.mouse.y = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool Emgui_regionHitSlider(const EmguiControlInfo* control) {
    const int mouse_x = g_emguiGuiState.mouse.x;
    const int mouse_y = g_emguiGuiState.mouse.y;

    const int slider_thumb_x = control->sliderThumbX + 4;
    const int slider_thumb_y = control->sliderThumbY + 4;

    if (mouse_x < slider_thumb_x || mouse_y < slider_thumb_y || mouse_x >= slider_thumb_x + control->sliderThumbWidth ||
        mouse_y >= slider_thumb_y + control->sliderThumbHeight) {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void fillGrad(uint32_t color0, uint32_t color1, int x, int y, int w, int h, int stipple) {
    const int active_layer = s_activeLayer;
    struct DrawFillCommand* command = LinearAllocator_allocZero(&s_allocator, struct DrawFillCommand);

    command->x = x;
    command->y = y;
    command->width = w;
    command->height = h;
    command->color0 = color0;
    command->color1 = color1;
    command->stipple = stipple;

    if (!s_renderData.layers[active_layer].fillCommands) {
        s_renderData.layers[active_layer].fillCommands = command;  // first command
        s_renderData.layers[active_layer].fillCommandsTail = command;
    } else {
        s_renderData.layers[active_layer].fillCommandsTail->next = command;
        s_renderData.layers[active_layer].fillCommandsTail = command;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_fill(uint32_t color, int x, int y, int w, int h) {
    Emgui_fillGrad(color, color, x, y, w, h);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_fillStipple(uint32_t color, int x, int y, int w, int h) {
    fillGrad(color, color, x, y, w, h, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_fillGrad(uint32_t color0, uint32_t color1, int x, int y, int w, int h) {
    fillGrad(color0, color1, x, y, w, h, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_drawBorder(uint32_t color0, uint32_t color1, int x, int y, int w, int h) {
    const uint32_t size = 2;

    Emgui_fill(color0, x, y, size, h);
    Emgui_fill(color0, x, y, w, size);
    Emgui_fill(color1, x + w, y, size, h + size);
    Emgui_fill(color1, x, y + h, w, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_textLabelXY(const char* text, int x, int y) {
    Emgui_drawText(text, x, y, 0xffffffff);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float floatClamp(float value, float low, float high) {
    if (value < low)
        value = low;

    if (value > high)
        value = high;

    return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Emgui_slider(int x, int y, int w, int h, int start, int end, int largeVal, enum EmguiSliderDirection dir,
                  int itemSpace, int* ivalue) {
    // int thumbPosition = 0;
    int controlId = 0;
    float value = (float)*ivalue;
    float range;
    int left, height;
    int thumb_width = 0, thumb_height = 0;
    EmguiControlInfo* control = 0;

    // Setup the control
    controlId = (int)g_controlId++;
    control = &g_controls[controlId];
    control->type = EMGUI_DRAWTYPE_SLIDER;
    control->x = x;
    control->y = y;
    control->width = w;
    control->height = h;

    // considering how much stuff we have have in the slider we need to calculate how much space the scolling area
    // actually is so we can resize the thumb

    range = (float)(end - start);

    if (dir == EMGUI_SLIDERDIR_HORIZONTAL) {
        float lw = (float)largeVal;
        float fw = (float)w;
        thumb_width = (int)((lw * fw) / (range + 1.0f));
        left = x + (int)((w - thumb_width) * (value / (float)end));

        if (thumb_width < 0)
            thumb_width = 2;

        control->sliderThumbX = left;
        control->sliderThumbY = y + 1;
        control->sliderThumbWidth = thumb_width;
        control->sliderThumbHeight = h;
    } else {
        float lw = (float)largeVal;
        float fh = (float)h;
        thumb_height = (int)((lw * fh) / (range + 1.0f));
        height = y + (int)((h - thumb_height) * (value / (float)end));

        if (thumb_height < 0)
            thumb_height = 2;

        control->sliderThumbX = x + 1;
        control->sliderThumbY = height;
        control->sliderThumbWidth = w;
        control->sliderThumbHeight = thumb_height;
    }

    Emgui_fill(Emgui_color32(255, 255, 255, 255), control->sliderThumbX, control->sliderThumbY,
               control->sliderThumbWidth, control->sliderThumbHeight);

    if (Emgui_regionHitSlider(control)) {
        g_emguiGuiState.hotItem = controlId;
        if (g_emguiGuiState.activeItem == 0 && g_emguiGuiState.mouse.down)
            g_emguiGuiState.activeItem = controlId;
    }

    if (g_emguiGuiState.activeItem == controlId) {
        if (dir == EMGUI_SLIDERDIR_HORIZONTAL) {
            int mouseRelative = g_emguiGuiState.mouse.x - (x + thumb_width / 2);
            mouseRelative = eclampi(mouseRelative, 0, w - thumb_width - 1);

            value = ((float)mouseRelative / (float)(w - thumb_width)) * (float)end;
            *ivalue = (int)value;

            // printf("%f %d (clamp %d) %f\n", (float)mouseRelative / (float)(w), mouseRelative, (w - thumb_width - 1),
            // value);
        } else {
            int mouseRelative = g_emguiGuiState.mouse.y - (y + thumb_height / 2);
            mouseRelative = eclampi(mouseRelative, 0, h - thumb_height - 1);

            value = ((float)mouseRelative / (float)(h - thumb_height)) * (float)end;
            *ivalue = (int)value;
        }

        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_textLabel(const char* text) {
    uint32_t controlId = 0;
    EmguiControlInfo* control = 0;

    // Setup the control
    controlId = g_controlId++;
    control = &g_controls[controlId];
    control->type = EMGUI_DRAWTYPE_TEXT;
    control->x = g_placementInfo.x;
    control->y = g_placementInfo.y;
    control->width = 0;   // getTextSize(text);
    control->height = 9;  // fixme
    control->text = LinearAllocator_allocString(&s_allocator, text);
    control->color = 0;
    control->fontId = g_currentFont;

    switch (g_placementInfo.state) {
        case PLACEMENTSTATE_NONE: {
            break;
        }

        case PLACEMENTSTATE_HORIZONAL: {
            g_placementInfo.x += control->width;
            break;
        }

        case PLACEMENTSTATE_VERTICAL: {
            g_placementInfo.y += 9;  // TODO: Use correct size from font
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawImage(uint64_t imageId, uint32_t color, int x, int y, int w, int h) {
    const int active_layer = s_activeLayer;
    struct DrawImageCommand* command = LinearAllocator_alloc(&s_allocator, struct DrawImageCommand);

    command->next = 0;
    command->imageId = imageId;
    command->x = x;
    command->y = y;
    command->width = w;
    command->height = h;
    command->color = color;

    if (!s_renderData.layers[active_layer].imageCommands) {
        s_renderData.layers[active_layer].imageCommands = command;
        s_renderData.layers[active_layer].imageCommandsTail = command;
    } else {
        s_renderData.layers[active_layer].imageCommandsTail->next = command;
        s_renderData.layers[active_layer].imageCommandsTail = command;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_drawTexture(uint64_t imageId, uint32_t color, int x, int y, int w, int h) {
    drawImage(imageId, color, x, y, w, h);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawText(const char* text, bool flipped, int x, int y, uint32_t color) {
    struct DrawTextCommand* temp;
    struct DrawTextCommand* command = LinearAllocator_alloc(&s_allocator, struct DrawTextCommand);
    command->x = x;
    command->y = y;
    command->text = LinearAllocator_allocString(&s_allocator, text);
    command->color = color;
    command->flipped = flipped;

    assert(g_currentFont < EMGUI_MAX_FONTS);

    temp = s_renderData.layers[s_activeLayer].textCommands[g_currentFont];
    s_renderData.layers[s_activeLayer].textCommands[g_currentFont] = command;
    command->next = temp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_drawTextFlipped(const char* text, int x, int y, uint32_t color) {
    drawText(text, true, x, y, color);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_drawText(const char* text, int x, int y, uint32_t color) {
    drawText(text, false, x, y, color);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_drawChar(char c, int x, int y, uint32_t color) {
    char t[2];
    t[0] = c;
    t[1] = 0;
    Emgui_drawText(t, x, y, color);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void* readFileToMemory(const char* filename) {
    int size;
    void* fileBuffer;
    FILE* file;

    file = fopen(filename, "rb");
    if (!file)
        return false;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (!(fileBuffer = malloc(size))) {
        fclose(file);
        return false;
    }

    int v = fread(fileBuffer, 1, size, file);
    fclose(file);
    (void)v;

    return fileBuffer;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t Emgui_loadFont(const char* ttfFontname, float fontHeight) {
    uint32_t fontId = 0;
    struct LoadedFont* loadedFont;
    uint8_t* fontBitmap = 0;
    void* ttfFontBuffer = readFileToMemory(ttfFontname);

    if (!ttfFontBuffer)
        return false;

    if (g_loadedFontsCount > MAX_FONTS)
        return false;

    if (!(fontBitmap = malloc(EM_FONTBITMAP_WIDTH * EM_FONTBITMAP_HEIGHT)))
        return false;

    fontId = g_loadedFontsCount++;

    loadedFont = &g_loadedFonts[fontId];
    memset(loadedFont, 0, sizeof(LoadedFont));
    strncpy(loadedFont->name, ttfFontname, 512);

    // TODO: Support support different texturesizes and utf8/non-ascii chars?

    stbtt_BakeFontBitmap(ttfFontBuffer, 0, fontHeight, fontBitmap, EM_FONTBITMAP_WIDTH, EM_FONTBITMAP_HEIGHT, 32, 96,
                         loadedFont->cData);

    loadedFont->handle = EMGFXBackend_createFontTexture(fontBitmap, EM_FONTBITMAP_WIDTH, EM_FONTBITMAP_HEIGHT);

    free(ttfFontBuffer);
    free(fontBitmap);

    return fontId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setFont(uint32_t fontId) {
    g_currentFont = fontId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setDefaultFont() {
    Emgui_setFont(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t genericImageControl(const char* filename) {
    uint32_t controlId = 0;
    EmguiControlInfo* control = 0;
    struct EmguiImage* image = 0;  // loadImage(filename);

    if (!image)
        return (uint32_t)~0;

    // Setup the control

    controlId = g_controlId++;
    control = &g_controls[controlId];
    control->type = EMGUI_DRAWTYPE_IMAGE;
    control->x = g_placementInfo.x;
    control->y = g_placementInfo.y;
    // control->width = image->width;
    // control->height = image->height;
    // control->imageData = image;

    // updatePlacement();

    switch (g_placementInfo.state) {
        case PLACEMENTSTATE_NONE: {
            break;
        }

        case PLACEMENTSTATE_HORIZONAL: {
            // g_placementInfo.x += image->width;
            break;
        }

        case PLACEMENTSTATE_VERTICAL: {
            // g_placementInfo.y += image->height;
            break;
        }
    }

    return controlId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_staticImage(const char* filename) {
    genericImageControl(filename);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
static void highlightControl(EmguiControlInfo* control)
{
    Emgui_fill(0xb0ffffff, control->x, control->y, control->width, control->height);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Emgui_buttonImage(const char* filename) {
    EmguiControlInfo* control;
    int controlId = 0;

    controlId = genericImageControl(filename);

    if (controlId == ~0)
        return false;

    control = &g_controls[controlId];

    if (Emgui_regionHit(control)) {
        g_emguiGuiState.hotItem = controlId;
        if (g_emguiGuiState.activeItem == 0 && g_emguiGuiState.mouse.down)
            g_emguiGuiState.activeItem = controlId;

        // highlightControl(control);
    }

    if (g_emguiGuiState.mouse.down == 0 && g_emguiGuiState.hotItem == controlId &&
        g_emguiGuiState.activeItem == controlId)
        return true;

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Emgui_buttonCoords(const char* text, uint32_t color, int x, int y, int width, int height) {
    EmguiControlInfo* control;
    int controlId = 0;
    uint32_t textSize = Emgui_getTextSize(text);
    uint32_t size_x = textSize & 0xffff;
    uint32_t size_y = textSize >> 16;

    Emgui_drawBorder(color, color, x, y, width, height);

    Emgui_textLabelXY(text, x + (width - size_x) / 2, y + (height - size_y) / 2);

    controlId = (int)g_controlId++;
    control = &g_controls[controlId];

    control->x = x;
    control->y = y;
    control->width = width;
    control->height = height;
    control->color = color;

    if (Emgui_regionHit(control)) {
        g_emguiGuiState.hotItem = controlId;
        if (g_emguiGuiState.activeItem == 0 && g_emguiGuiState.mouse.down)
            g_emguiGuiState.activeItem = controlId;
    }

    if (g_emguiGuiState.mouse.down == 0 && g_emguiGuiState.hotItem == controlId &&
        g_emguiGuiState.activeItem == controlId) {
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_radioButtonImage(void* image0, int size0, void* image1, int size1, enum EmguiMemoryLocation location,
                            uint32_t color, int x, int y, bool* stateIn) {
    EmguiControlInfo* control;
    bool state = *stateIn;
    int controlId = 0;
    int w0 = -1, h0 = -1;
    int w1 = -1, h1 = -1;
    int w, h;
    uint64_t i0 = loadImage(&w0, &h0, image0, size0, location);
    uint64_t i1 = loadImage(&w1, &h1, image1, size1, location);

    if ((uint64_t)~0 == i0 || (uint64_t)~0 == i1) {
        printf("EMGUI: (radioButton) Unable to create due to fail of loading image(s)\n");
        return;
    }

    controlId = (int)g_controlId++;
    control = &g_controls[controlId];

    w = state ? w0 : w1;
    h = state ? h0 : h1;

    control->x = x;
    control->y = y;
    control->width = w;
    control->height = h;
    control->color = color;

    if (Emgui_regionHit(control)) {
        g_emguiGuiState.hotItem = controlId;
        if (g_emguiGuiState.activeItem == 0 && g_emguiGuiState.mouse.down)
            g_emguiGuiState.activeItem = controlId;
    }

    if (g_emguiGuiState.mouse.down == 0 && g_emguiGuiState.hotItem == controlId &&
        g_emguiGuiState.activeItem == controlId) {
        state = !state;
    }

    if (state)
        drawImage(i0, color, x, y, w0, h0);
    else
        drawImage(i1, color, x, y, w1, h1);

    *stateIn = state;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_end() {
    if (g_emguiGuiState.mouse.down == 0) {
        g_emguiGuiState.activeItem = 0;
    } else {
        if (g_emguiGuiState.activeItem == 0)
            g_emguiGuiState.activeItem = -1;
    }

    g_emguiGuiState.keyCode = 0;

    EMGFXBackend_render();

    // Rewind the linear allocator

    LinearAllocator_rewind(&s_allocator, s_rewindPoint);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_editBoxXY(int x, int y, int width, int height, int bufferLength, char* buffer) {
    EmguiControlInfo* control;
    int controlId = 0;
    const int keyCode = g_emguiGuiState.keyCode;
    uint32_t color = Emgui_color32(200, 200, 127, 255);

    controlId = (int)g_controlId++;
    control = &g_controls[controlId];
    control->x = x;
    control->y = y;
    control->width = width;
    control->height = height;
    control->color = color;

    if (Emgui_regionHit(control)) {
        g_emguiGuiState.hotItem = controlId;

        if (g_emguiGuiState.activeItem == 0 && g_emguiGuiState.mouse.down) {
            g_emguiGuiState.activeItem = controlId;
            g_emguiGuiState.kbdItem = controlId;
        }
    }

    // if we have keyboard active on this control we need to modify the text

    if (g_emguiGuiState.kbdItem == controlId && keyCode != 0) {
        int len = (int)strlen(buffer);

        switch (keyCode) {
            case EMGUI_KEY_ESC: {
                g_emguiGuiState.kbdItem = -1;
                g_emguiGuiState.keyCode = 0;
                break;
            }

            case EMGUI_KEY_TAB: {
                if (g_emguiGuiState.keyMod & EMGUI_KEY_SHIFT) {
                    if (controlId - 1 <= 0) {
                        g_emguiGuiState.kbdItem = -1;
                        g_emguiGuiState.keyCode = 0;
                    } else {
                        g_emguiGuiState.kbdItem = controlId - 1;
                        g_emguiGuiState.keyCode = 0;
                    }
                    break;
                } else {
                    // This is a small hack to toggle between the edit boxes using tab
                    if (controlId + 1 >= 6) {
                        g_emguiGuiState.kbdItem = -1;
                        g_emguiGuiState.keyCode = 0;
                    } else {
                        g_emguiGuiState.kbdItem = controlId + 1;
                        g_emguiGuiState.keyCode = 0;
                    }
                }

                break;
            }

            case EMGUI_KEY_ENTER: {
                g_emguiGuiState.kbdItem = -1;
                g_emguiGuiState.keyCode = 0;
                break;
            }

            case EMGUI_KEY_BACKSPACE: {
                buffer[emaxi(len - 1, 0)] = 0;
                break;
            }

            default: {
                // Rocket hack:
                // Currenty for rocket we only need 0-9 as input (for the edit field
                // so we only update here if those are the keys
                int offset = emini(bufferLength - 2, len);
                if (keyCode >= '0' && keyCode <= '9') {
                    buffer[offset + 0] = (char)keyCode;
                    buffer[offset + 1] = 0;
                }
                break;
            }
        }
    }

    Emgui_fillGrad(Emgui_color32(70, 70, 70, 255), Emgui_color32(30, 30, 30, 255), x, y, width, height);
    Emgui_drawText(buffer, x + 2, y, color);

    if (g_emguiGuiState.kbdItem == controlId) {
        int text_size = Emgui_getTextSize(buffer) & 0xffff;
        uint32_t color0 = Emgui_color32(200, 200, 127, 127);
        uint32_t color1 = Emgui_color32(200, 200, 127, 127);
        Emgui_drawBorder(color0, color1, x, y, width, height);
        Emgui_fill(Emgui_color32(200, 200, 127, 200), x + text_size + 2, y + 2, 2, height - 2);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_sendKeyinput(int keyCode, int modifier) {
    g_emguiGuiState.keyCode = keyCode;
    g_emguiGuiState.keyMod = modifier;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setMousePos(int posX, int posY) {
    g_emguiGuiState.mouse.x = posX;
    g_emguiGuiState.mouse.y = posY;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setMouseLmb(int state) {
    g_emguiGuiState.mouse.down = state;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Emgui_hasKeyboardFocus() {
    return g_emguiGuiState.kbdItem != -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setFirstControlFocus() {
    g_emguiGuiState.kbdItem = 1;
    g_emguiGuiState.keyCode = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_resetFocus() {
    g_emguiGuiState.kbdItem = -1;
    g_emguiGuiState.keyCode = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setLayer(int layer) {
    s_activeLayer = layer;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Emgui_setScissor(int x, int y, int w, int h) {
    const int active_layer = s_activeLayer;
    s_renderData.layers[active_layer].scissor.x = x;
    s_renderData.layers[active_layer].scissor.y = y;
    s_renderData.layers[active_layer].scissor.width = w;
    s_renderData.layers[active_layer].scissor.height = h;
}
