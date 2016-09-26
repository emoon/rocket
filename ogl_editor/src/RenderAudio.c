#include "RenderAudio.h"
#include "TrackData.h"
#include <emgui/Emgui.h>
#include <emgui/GFXBackend.h>
#include <stdio.h>

#define TEXTURE_SHIFT 10
#define TEXTURE_HEIGHT (1 << TEXTURE_SHIFT)

typedef struct RenderData
{
    uint64_t textureIds[128];
    uint32_t* textureData[128];
    int textureCount;
} RenderData;

static RenderData s_data;

void fillColor(uint32_t* dest, uint32_t color, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < 128; ++x) {
            *dest++ = color;
        }
    }
}

void RenderAudio_update(struct TrackData* trackData, int xPos, int yPos, int rowCount, int rowOffset, int rowSpacing, int endY)
{
    uint32_t* fftData = trackData->musicData.fftData;

    if (!fftData)
        return;

    // first calculate the total number of textures we need (we allocate 128 x 1024 textures to be compatible to fairly low-end GPUs)

    int count = (int)(((float)endY) / (float)TEXTURE_HEIGHT) + 1;

    if (s_data.textureCount < count)
    {
        for (int i = s_data.textureCount; i < count; ++i)
        {
            s_data.textureIds[i] = EMGFXBackend_createTexture(0, 128, TEXTURE_HEIGHT, 4);
            s_data.textureData[i] = malloc(128 * TEXTURE_HEIGHT * 4);
        }

        s_data.textureCount = count;
    }

    int texturePos = 0;

	if (rowOffset < 0)
	{
	    rowOffset = -rowOffset;

	    for (int i = 0; i < rowOffset; ++i) {
            const int textureIndex = texturePos >> 17;
            const int textureOffset = texturePos & 0x1ffff;

            uint32_t* textureDest = &s_data.textureData[textureIndex][textureOffset];

            fillColor(textureDest, Emgui_color32(0, 0, 0, 255), rowSpacing);
            texturePos += 128 * rowSpacing;
	    }

		yPos += rowSpacing * rowOffset;
        rowOffset = 0;
	}

	for (int i = 0; i < rowCount; ++i)
	{
        const int textureIndex = texturePos >> 17;
        const int textureOffset = texturePos & 0x1ffff;

	    uint32_t* textureDest = &s_data.textureData[textureIndex][textureOffset];

	    //printf("i %d - textureIndex %d - textureOffset %d (%d)\n", i, textureIndex, textureOffset, i * rowSpacing);

        memcpy(textureDest, &fftData[rowOffset * rowSpacing * 128], 128 * 4 * rowSpacing);

        texturePos += 128 * rowSpacing;
        yPos += rowSpacing;
        rowOffset++;

		if (yPos > endY)
			break;
	}

    for (int i = 0; i < s_data.textureCount; ++i) {
        EMGFXBackend_updateTexture(s_data.textureIds[i], 128, TEXTURE_HEIGHT, s_data.textureData[i]);
    }
}


void RenderAudio_render(int xPos, int startY, int endY)
{
	Emgui_setLayer(2);
	Emgui_setScissor(xPos, startY, 128, endY);

    for (int i = 0; i < s_data.textureCount; ++i)
    {
        Emgui_drawTexture(s_data.textureIds[i], Emgui_color32(255, 255, 255, 255), xPos, startY, 128, TEXTURE_HEIGHT);
        startY += TEXTURE_HEIGHT;
    }

	Emgui_setLayer(0);
}

