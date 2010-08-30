/* Copyright (C) 2010 Erik Faye-Lund and Egbert Teeselink
 * For conditions of distribution and use, see copyright notice in COPYING
 */

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "../sync/sync.h"

#define BPM 150 /* beats per minute */
#define RPB 8   /* rows per beat */
#define ROW_RATE ((BPM / 60.0) * RPB)

double sdl_get_row()
{
/*	return Mix_GetMusicPosition() * ROW_RATE; */
	return 0;
}

#ifndef SYNC_PLAYER

void sdl_pause(void *d, int flag)
{
	if (flag)
		Mix_PauseMusic();
	else
		Mix_ResumeMusic();
}

void sdl_set_row(void *d, int row)
{
	Mix_SetMusicPosition(row / ROW_RATE);
}

int sdl_is_playing(void *d)
{
	return !!Mix_PlayingMusic();
}

struct sync_cb sdl_cb = {
	sdl_pause,
	sdl_set_row,
	sdl_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

void die(const char *fmt, ...)
{
	char temp[4096];
	va_list va;
	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp), fmt, va);
	va_end(va);

	fprintf(stderr, "*** error: %s\n", temp);
	exit(EXIT_FAILURE);
}

const unsigned int width  = 800;
const unsigned int height = 600;

int main(int argc, char *argv[])
{
	int done = 0;
	struct sync_device *rocket;
	Mix_Music *music;
	const struct sync_track *clear_r, *clear_g, *clear_b;
	const struct sync_track *cam_rot, *cam_dist;

	/* initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		die("failed to init SDL!");
	atexit(SDL_Quit);

	if (!SDL_SetVideoMode(width, height, 32, SDL_OPENGL))
		die("failed to set video mode!");

#if MIX_MAJOR_VERSION > 1 || \
    (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION > 2) || \
    (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL >= 10)
	/* init SDL_mixer */
	if (!Mix_Init(MIX_INIT_OGG))
		die("failed to init SDL_mixer: %s", Mix_GetError());
	atexit(Mix_Quit);
#endif

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096))
		die("failed to open audio output: %s", Mix_GetError());

	music = Mix_LoadMUS("tune.ogg");
	if (!music)
		die("failed to open tune: %s", Mix_GetError());

	rocket = sync_create_device("sync");
	if (!rocket)
		die("out of memory?");

#ifndef SYNC_PLAYER
	sync_set_callbacks(rocket, &sdl_cb, (void *)music);
	if (sync_connect(rocket, "10.0.1.10", SYNC_DEFAULT_PORT))
		die("failed to connect to host");
#endif

	/* get tracks */
	clear_r = sync_get_track(rocket, "clear.r");
	clear_g = sync_get_track(rocket, "clear.g");
	clear_b = sync_get_track(rocket, "clear.b");
	cam_rot = sync_get_track(rocket, "cam.rot"),
	cam_dist = sync_get_track(rocket, "cam.dist");

	/* let's roll! */
	if (Mix_PlayMusic(music, 0) < 0)
		die("failed to play music: %s", Mix_GetError());

	while (!done) {
		SDL_Event event;
		double row = sdl_get_row();
		float rot, dist;

#ifndef SYNC_PLAYER
		if (sync_update(rocket, (int)floor(row)))
			sync_connect(rocket, "10.0.1.10", SYNC_DEFAULT_PORT);
#endif

		/* draw */

		glClearColor(
			sync_get_val(clear_r, row),
			sync_get_val(clear_g, row),
			sync_get_val(clear_b, row),
			0.0
		);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
		    GL_STENCIL_BUFFER_BIT);

		rot = sync_get_val(cam_rot, row);
		dist = sync_get_val(cam_dist, row);

		glMatrixMode(GL_PROJECTION);
		gluPerspective(60.0, (double)width / height, 0.1, 1000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(
		    sin(rot) * dist, cos(rot) * dist, 0.0,
		    0.0, 0.0, 0.0,
		    0.0, 1.0, 0.0
		);

		glDisable(GL_CULL_FACE);
		glBegin(GL_QUADS);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f( 1.0f, -1.0f);
		glVertex2f( 1.0f,  1.0f);
		glVertex2f(-1.0f,  1.0f);
		glEnd();

		SDL_GL_SwapBuffers();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					done = 1;
			case SDL_QUIT:
				done = 1;
			}
		}
	}

	return 0;
}
