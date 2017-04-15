#include "../include/emgui/GFXBackend.h"
#include "../Emgui_internal.h"
#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#elif defined(EMGUI_UNIX)
#include <GL/gl.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#endif
#include <stdlib.h>
#include <stdio.h>

/*
#define GL_CHECK(x) do { \
    x; \
    GLenum glerr = glGetError(); \
    if (glerr != GL_NO_ERROR) { \
        printf("EMGUI: OpenGL error: %d, file: %s, line: %d", glerr, __FILE__, __LINE__); \
    } \
} while (0)
*/

#define GL_CHECK(x) x;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void setColor(uint32_t color)
{
	// TODO: Figure out why glColor4b doesn't work here :/

	glColor4f(((color >> 0) & 0xff) * 1.0f / 255.0f,
			  ((color >> 8) & 0xff) * 1.0f / 255.0f,
			  ((color >> 16) & 0xff) * 1.0f / 255.0f,
			  ((color >> 24) & 0xff) * 1.0f / 255.0f);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EMGFXBackend_create()
{
	// set the background colour
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EMGFXBackend_destroy()
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EMGFXBackend_updateViewPort(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
}

static GLuint texId;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint64_t EMGFXBackend_createFontTexture(void* fontBuffer, int width, int height)
{
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, fontBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return (uint64_t)texId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint64_t EMGFXBackend_createTexture(void* imageBuffer, int w, int h, int comp)
{
	int format = GL_RGBA;

	GL_CHECK(glGenTextures(1, &texId));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, texId));

	switch (comp)
	{
		case 1 : format = GL_ALPHA; break;
		case 3 : format = GL_RGB; break;
		case 4 : format = GL_RGBA; break;
	}

    if (imageBuffer) {
	    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, imageBuffer));
	}

	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));

	return (uint64_t)texId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EMGFXBackend_updateTexture(uint64_t texId, int w, int h, void* data)
{
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, (GLuint)texId));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawText(struct DrawTextCommand* commands, int fontId)
{
	const LoadedFont* font = &g_loadedFonts[fontId];

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, (GLuint)font->handle);

	//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);

	while (commands)
	{
		const char* text = commands->text;
		float x = (float)commands->x;
		float y = (float)commands->y;
		const bool flipped = commands->flipped;
		setColor(commands->color);

		if (font->altLookup)
		{
			char c = *text++;

			while (c != 0)
			{
				int offset = c - 32;
				float s0 = (float)font->altLookup[(offset * 2) + 0];
				float t0 = (float)font->altLookup[(offset * 2) + 1];
				float s1 = s0 + 8.0f;
				float t1 = t0 + 8.0f;
				float x0 = x;
				float y0 = y;
				float x1 = x + 8.0f;
				float y1 = y + 8.0f;

				s0 *= 1.0f / 128.0f;
				t0 *= 1.0f / 128.0f;
				s1 *= 1.0f / 128.0f;
				t1 *= 1.0f / 128.0f;

				glTexCoord2f(s0, t0); glVertex2f(x0, y0);
				glTexCoord2f(s1, t0); glVertex2f(x1, y0);
				glTexCoord2f(s1, t1); glVertex2f(x1, y1);
				glTexCoord2f(s0, t1); glVertex2f(x0, y1);

				c = *text++;
				x += 8.0f;
			}
		}
		else if (font->layout)
		{
			float s_scale = 1.0f / (float)font->width;
			float t_scale = 1.0f / (float)font->height;
			const int range_start = font->rangeStart;
			const int range_end = font->rangeEnd;
			int c = (unsigned char)*text++;

			while (c != 0)
			{
				if (c >= range_start && c < range_end)
				{
					int offset = c - range_start;
					EmguiFontLayout* info = &font->layout[offset];

					float s0 = info->x;
					float t0 = info->y;
					float s1 = (float)(info->x + info->width);
					float t1 = (float)(info->y + info->height);

					if (flipped)
					{
						float x0 = x + (float)info->yoffset;
						float y0 = y + (float)info->xoffset - info->xadvance;
						float x1 = x0 + (float)info->height;
						float y1 = y0 + (float)info->width;

						s0 *= s_scale;
						s1 *= s_scale;
						t0 *= t_scale;
						t1 *= t_scale;

						glTexCoord2f(s1, t0); glVertex2f(x0, y0);
						glTexCoord2f(s1, t1); glVertex2f(x1, y0);
						glTexCoord2f(s0, t1); glVertex2f(x1, y1);
						glTexCoord2f(s0, t0); glVertex2f(x0, y1);

						y -= (float)info->xadvance;
					}
					else
					{
						float x0 = x + (float)info->xoffset;
						float y0 = y + (float)info->yoffset;
						float x1 = x0 + (float)info->width;
						float y1 = y0 + (float)info->height;

						s0 *= s_scale;
						s1 *= s_scale;
						t0 *= t_scale;
						t1 *= t_scale;

						glTexCoord2f(s0, t0); glVertex2f(x0, y0);
						glTexCoord2f(s1, t0); glVertex2f(x1, y0);
						glTexCoord2f(s1, t1); glVertex2f(x1, y1);
						glTexCoord2f(s0, t1); glVertex2f(x0, y1);

						x += (float)info->xadvance;
					}
				}

				c = *text++;
			}

		}
		else
		{
			while (*text)
			{
				int c = (unsigned char)*text;
				if (c >= 32 && c < 128)
				{
					stbtt_aligned_quad q;
					stbtt_GetBakedQuad((stbtt_bakedchar*)font->cData, 512,512, c-32, &x, &y, &q, 1);

					glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
					glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
					glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
					glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);

				}
				++text;
			}
		}

		commands = commands->next;
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawFill(struct DrawFillCommand* command)
{
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);

	while (command)
	{
		const float x = (float)command->x;
		const float y = (float)command->y;
		const float w = (float)command->width;
		const float h = (float)command->height;

		if (command->stipple)
		{
			command = command->next;
			continue;
		}

		setColor(command->color0);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		setColor(command->color1);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);

		command = command->next;
	}

	glEnd();

	glDisable(GL_POLYGON_STIPPLE);
	glDisable(GL_BLEND);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawFillStipple(struct DrawFillCommand* command)
{
	glEnable(GL_POLYGON_STIPPLE);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);

	while (command)
	{
		const float x = (float)command->x;
		const float y = (float)command->y;
		const float w = (float)command->width;
		const float h = (float)command->height;

		if (!command->stipple)
		{
			command = command->next;
			continue;
		}

		setColor(command->color0);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		setColor(command->color1);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);

		command = command->next;
	}

	glEnd();

	glDisable(GL_POLYGON_STIPPLE);
	glDisable(GL_BLEND);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawImage(struct DrawImageCommand* command)
{
	uint64_t lastTexture = command->imageId;

	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, (GLuint)lastTexture);

	glBegin(GL_QUADS);

	// TODO: Sorting of images type up-front

	while (command)
	{
		const float x = (float)command->x;
		const float y = (float)command->y;
		const float w = (float)command->width;
		const float h = (float)command->height;

		if (command->imageId != (int64_t)lastTexture)
		{
			glEnd();
			glBindTexture(GL_TEXTURE_2D, (GLuint)command->imageId);
			glBegin(GL_QUADS);
			lastTexture = command->imageId;
		}

		setColor(command->color);

		glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + h);

		command = command->next;
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

extern struct RenderData s_renderData;
extern void swapBuffers();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EMGFXBackend_render()
{
	int i = 0, layerIter = 0;

	// Not sure how to handle the clears, maybe send ClearScreen flag to Emgui_end()?

    glClear(GL_COLOR_BUFFER_BIT);
    // prepare for primitive drawing
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (layerIter = 0; layerIter < EMGUI_LAYER_COUNT; ++layerIter)
	{
		struct DrawLayer* layer = &s_renderData.layers[layerIter];

		if (layer->scissor.width > 0 && layer->scissor.height > 0)
		{
			glEnable(GL_SCISSOR_TEST);
			glScissor(layer->scissor.x, layer->scissor.y,
					  layer->scissor.width, layer->scissor.height);
		}

		drawFill(layer->fillCommands);
		drawFillStipple(layer->fillCommands);

		for (i = 0; i < EMGUI_MAX_FONTS; ++i)
		{
			if (layer->textCommands[i])
				drawText(layer->textCommands[i], i);
		}

		if (layer->imageCommands)
			drawImage(layer->imageCommands);

		if (layer->scissor.width > 0 && layer->scissor.height > 0)
		{
			glDisable(GL_SCISSOR_TEST);
		}
	}

	swapBuffers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EMGFXBackend_setStippleMask(unsigned char* mask)
{
	glPolygonStipple(mask);
}



