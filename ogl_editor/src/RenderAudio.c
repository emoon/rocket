#include "RenderAudio.h"
#include "TrackData.h"
#include <emgui/Emgui.h>
#include <emgui/GFXBackend.h>
#include <stdio.h>
#include <math.h>

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


	// Notice: This code assumes FFT has 100 samples per sec

    float rowsPerSec = (float)trackData->bpm * ((float)trackData->rowsPerBeat) / 60.0;
    const int fftSampes = trackData->musicData.sampleCount;

    //printf("rowsPerSec %f, fftStep %f\n", rowsPerSec, fftStep);

	// Calculate the start pos of the fft data and interpolation

	for (int i = 0; i < rowCount; ++i)
	{
	    // TODO: Optimize
        float rowSec0 = (float)rowOffset / rowsPerSec;
        float rowSec1 = ((float)(rowOffset + 1)) / rowsPerSec;
        float step = ((rowSec1 * 100.f) - (rowSec0 * 100.0f)) / rowSpacing;

        printf("rowOffset %d time %f - %f\n", rowOffset, rowSec0, step);

        float fftPos = rowSec0 * 100.f;

	    for (int r = 0; r < rowSpacing; ++r)
	    {
            const int textureIndex = texturePos >> 17;
            const int textureOffset = texturePos & 0x1ffff;
	        uint32_t* textureDest = &s_data.textureData[textureIndex][textureOffset];

	        int pos = (int)fftPos;

	        if (pos < fftSampes)
	        {
                memcpy(textureDest, &fftData[pos * 128], 128 * 4);
            }
            else
            {
                fillColor(textureDest, Emgui_color32(0, 0, 0, 255), 1);
            }

            fftPos += step;
            texturePos += 128;
	    }

	    //printf("i %d - textureIndex %d - textureOffset %d (%d)\n", i, textureIndex, textureOffset, i * rowSpacing);

        yPos += rowSpacing;
        rowOffset++;

		if (yPos > endY)
			break;
	}

    for (int i = 0; i < s_data.textureCount; ++i) {
        EMGFXBackend_updateTexture(s_data.textureIds[i], 128, TEXTURE_HEIGHT, s_data.textureData[i]);
    }
}

static void drawSinText(const char* text, float offset, float step, float scale, int xPos, int yPos)
{
    char c = *text++;
    float t = offset;

    while (c)
    {
        t = sinf(offset);
        Emgui_drawChar(c, xPos, yPos + (t * scale), Emgui_color32(255, 255, 255, 255));
        offset += step;
        xPos += 8;
        c = *text++;
    }
}

static float angle = 0;
static int extra_update_hack = 1;

void RenderAudio_render(struct TrackData* trackData, int xPos, int startY, int endY)
{
    int percentDone = trackData->musicData.percentDone;

    if (percentDone >= 1 && percentDone < 100)
    {
        char temp[1024];
        sprintf(temp, "    %02d%%", percentDone);

        xPos += 20;

        drawSinText("  Loading ", angle, 0.1f, 10.0f, xPos, startY);
        drawSinText("   while  ", angle, 0.1f, 10.0f, xPos, startY + 8);
        drawSinText("decruncing", angle, 0.1f, 10.0f, xPos, startY + 16);
        drawSinText(temp, angle, 0.1f, 10.0f, xPos, startY + 24);

        angle += 0.1f;  // we update at timed intervals so even if we *should* use delta time here it's likely fine.

        extra_update_hack = 1;
    }
    else
    {
        if (!extra_update_hack)
            trackData->musicData.percentDone = 0;

        Emgui_setLayer(2);
        Emgui_setScissor(xPos, startY, 128, endY);

        for (int i = 0; i < s_data.textureCount; ++i)
        {
            Emgui_drawTexture(s_data.textureIds[i], Emgui_color32(255, 255, 255, 255), xPos, startY, 128, TEXTURE_HEIGHT);
            startY += TEXTURE_HEIGHT;
        }

        Emgui_setLayer(0);

        extra_update_hack = 0;
    }
}

