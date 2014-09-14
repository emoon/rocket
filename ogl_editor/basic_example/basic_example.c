#include <stdio.h>
#include <math.h>
#if defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "../../sync/sync.h"

static struct sync_device *device;
#if !defined(SYNC_PLAYER)
static struct sync_cb cb;
#endif

#define sizeof_array(array) (int)(sizeof(array)/sizeof(array[0]))

const float bpm = 180.0f;
const float rpb = 8.0f;
float rps = 24.0f;// bpm / 60.0f * rpb; <- msvc cant compute this compile time... sigh
int audio_is_playing = 1;
int curtime_ms = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int row_to_ms_round(int row, float rps) 
{
	const float newtime = ((float)(row)) / rps;
	return (int)(floor(newtime * 1000.0f + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static float ms_to_row_f(int time_ms, float rps) 
{
	const float row = rps * ((float)time_ms) * 1.0f/1000.0f;
	return row;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int ms_to_row_round(int time_ms, float rps) 
{
	const float r = ms_to_row_f(time_ms, rps);
	return (int)(floor(r + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(SYNC_PLAYER)

static void xpause(void* data, int flag)
{
	(void)data;

	if (flag)
		audio_is_playing = 0;
	else
		audio_is_playing = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void xset_row(void* data, int row)
{
	int newtime_ms = row_to_ms_round(row, rps);
	curtime_ms = newtime_ms;
	(void)data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int xis_playing(void* data)
{
	(void)data;
	return audio_is_playing;
}

#endif //!SYNC_PLAYER

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int rocket_init(const char* prefix) 
{
	device = sync_create_device( prefix );
	if (!device) 
	{
		printf("Unable to create rocketDevice\n");
		return 0;
	}

#if !defined( SYNC_PLAYER )
	cb.is_playing = xis_playing;
	cb.pause = xpause;
	cb.set_row = xset_row;

	if (sync_connect(device, "localhost", SYNC_DEFAULT_PORT)) 
	{
		printf("Rocket failed to connect\n");
		return 0;
	}
#endif

	printf("Rocket connected.\n");

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int rocket_update()
{
	int row = 0;

	if (audio_is_playing)
		curtime_ms += 16; //...60hz or gtfo

#if !defined( SYNC_PLAYER )
	row = ms_to_row_round(curtime_ms, rps);
	if (sync_update(device, row, &cb, 0)) 
		sync_connect(device, "localhost", SYNC_DEFAULT_PORT);
#endif

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* s_trackNames[] = 
{
	"group0#track0",
	"group0#track1",
	"group0#track2",
	"group0#track3",
	"group1#track0",
	"group1#track1",
	"group1#track2",
};

static const struct sync_track* s_tracks[sizeof_array(s_trackNames)];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	int i;

	if (!rocket_init("data/sync"))
		return -1;

	for (i = 0; i < sizeof_array(s_trackNames); ++i)
		s_tracks[i] = sync_get_track(device, s_trackNames[i]);

	for (;;)
	{
		float row_f;

		rocket_update();

		row_f = ms_to_row_f(curtime_ms, rps);

		printf("current time %d\n", curtime_ms);

		for (i = 0; i < sizeof_array(s_trackNames); ++i)
			printf("%s %f\n", s_trackNames[i], sync_get_val(s_tracks[i], row_f));
	#if defined(WIN32)
		Sleep(16);
	#else
		usleep(16000);
	#endif
	}
}

