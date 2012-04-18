#include "OpenGLRenderer.h"
#include "Core/Types.h"
#include "../MicroknightFont.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

unsigned int s_fontTexture;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_create()
{
	// set the background colour
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_setFont(void* font, int w, int h)
{
	glGenTextures(1, &s_fontTexture);
	glBindTexture(GL_TEXTURE_2D, s_fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, font); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_updateViewPort(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_draw()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // prepare for primitive drawing
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glFlush();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_destroy()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GFXBackend_drawTextHorizontal(int x, int y, const char* text)
{
	const int charOffset = 32;
	char c = *text++;

 	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, s_fontTexture);
	glBegin(GL_QUADS);

	while (c != 0)
	{
		int offset = c - charOffset;
		uint16_t font_x = s_microknightLayout[offset].x;
		uint16_t font_y = s_microknightLayout[offset].y;
		const float font_0 = ((float)font_x) / 128.0f;
		const float font_1 = ((float)font_y) / 128.0f;
		const float font_2 = ((float)font_x + 8) / 128.0f;
		const float font_3 = ((float)font_y + 8) / 128.0f;

		glTexCoord2f(font_0, font_1);
		glVertex2i(x, y);
		glTexCoord2f(font_0, font_3);
		glVertex2i(x, y + 8);
		glTexCoord2f(font_2, font_3);
		glVertex2i(x + 8,y + 8);
		glTexCoord2f(font_2, font_1);
		glVertex2i(x + 8,y);

		c = *text++;
		x += 8;
	}

	glEnd();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GFXBackend_getTextPixelLength(const char* text)
{
	// hardcoded for now
	return (strlen(text) - 1) * 9;
}

void GFXBackend_drawLine(int x, int y, int xEnd, int yEnd)
{
	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
 	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glVertex2i(x, y);
	glVertex2i(xEnd, yEnd);
	glVertex2i(x + 1, y);
	glVertex2i(xEnd + 1, yEnd);
	glEnd();
}

struct RETrack;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GFXBackend_drawTrack(int xPos, int yPos, struct RETrack* track)
{
	/*
	int i, count;
	int trackWidth = GFXBackend_getTextPixelLength(track->name) + 4;

	// at least 30 pixels in width

	if (trackWidth < 30)
		trackWidth = 30;
    
    glColor3f(1.0f, 1.0f, 1.0f);
	GFXBackend_drawTextHorizontal(xPos, yPos, track->name);

	GFXBackend_drawLine(xPos + trackWidth, yPos, xPos + trackWidth, yPos + 800);	// TODO: proper y-size according to window size

	for (i = 2, count = 30; i < count; ++i)
	{
		drawRow(xPos, (i * 20));
	}

	return trackWidth + 8;
	*/

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGL_drawPosition(int row, int xPos, int yPos, int height)
{
	/*
	int i, count;

	// so yeah, we can make this smarter
	// TODO: remove sprintf (may be ok)
	// TODO: Build one list of quads instead of glVertex crap

	for (i = 0, count = (height - yPos); i < count; i += s_fontWidth)
	{
		char tempString[64];
		sprintf(tempString, "%08d", row);
	
		if (row & 7)
			glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		else
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		OpenGL_drawTextHorizontal(xPos, yPos, tempString);
		row++;
		yPos += s_fontWidth;
	}
	*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void quad(int x, int y, int width, int height)
{
	glVertex2i(x, y);
	glVertex2i(x, y + height);
	glVertex2i(x + width,y + height);
	glVertex2i(x + width,y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void drawRow(int x, int rowOffset)
{
	glBegin(GL_QUADS);
	quad(x, rowOffset, 8, 1);
	quad(x + 10, rowOffset, 8, 1);
	quad(x + 20, rowOffset, 8, 1);
	glEnd();
}




