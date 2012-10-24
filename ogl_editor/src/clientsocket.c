#include "ClientSocket.h"
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(disable: 4496)
#include <winsock2.h>
#pragma warning(default: 4496)
#endif

#if defined(_WIN32)
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "../../sync/base.h"
#include "../../sync/track.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

int s_socket = INVALID_SOCKET;
static bool s_paused = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	MAX_NAME_HASHES = 8192, // Should be enough with 8k tracks/channels :)
};

struct NameLookup
{
	uint32_t hashes[MAX_NAME_HASHES];
	uint32_t ids[MAX_NAME_HASHES];
	uint32_t count;
};

static struct NameLookup s_nameLookup;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// taken from:
// http://blade.nagaokaut.ac.jp/cgi-bin/scat.rb/ruby/ruby-talk/142054
//
// djb  :: 99.8601 percent coverage (60 collisions out of 42884)
// elf  :: 99.5430 percent coverage (196 collisions out of 42884)
// sdbm :: 100.0000 percent coverage (0 collisions out of 42884) (this is the algo used)
// ...

static uint32_t quickHash(const char* string)
{
	uint32_t c;
	uint32_t hash = 0;

	const uint8_t* str = (const uint8_t*)string;

	while ((c = *str++))
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash & 0x7FFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int findTrack(const char* name)
{
	uint32_t i, count, hash = quickHash(name);

	for (i = 0, count = s_nameLookup.count; i < count; ++i)
	{
		if (hash == s_nameLookup.hashes[i])
			return s_nameLookup.ids[i];
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ClientSocket_connected()
{
	return INVALID_SOCKET != s_socket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_disconnect()
{
#if defined(_WIN32)
	closesocket(s_socket);
#else
	close(s_socket);
#endif
	s_socket = INVALID_SOCKET;

	memset(s_nameLookup.ids, -1, sizeof(int) * s_nameLookup.count);
	s_nameLookup.count = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ClientSocket_recv(char* buffer, size_t length, int flags)
{
	int ret;

	if (!ClientSocket_connected())
		return false;

	if ((ret = recv(s_socket, buffer, (int)length, flags)) != (int)length)
	{
		ClientSocket_disconnect();
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ClientSocket_send(const char* buffer, size_t length, int flags)
{
	int ret;

	if (!ClientSocket_connected())
		return false;

	if ((ret = send(s_socket, buffer, (int)length, flags)) != (int)length)
	{
		ClientSocket_disconnect();
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ClientSocket_pollRead()
{
	if (!ClientSocket_connected())
		return false;

	return !!socket_poll(s_socket);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_sendSetKeyCommand(const char* trackName, const struct track_key* key)
{
	uint32_t track, row;
	uint8_t cmd = SET_KEY;
	int track_id;

	union {
		float f;
		uint32_t i;
	} v;

	track_id = findTrack(trackName);

	if (!ClientSocket_connected() || track_id == -1)
		return;

	track = htonl((uint32_t)track_id);
	row = htonl(key->row);

	v.f = key->value;
	v.i = htonl(v.i);

	assert(key->type < KEY_TYPE_COUNT);

	ClientSocket_send((char *)&cmd, 1, 0);
	ClientSocket_send((char *)&track, sizeof(track), 0);
	ClientSocket_send((char *)&row, sizeof(row), 0);
	ClientSocket_send((char *)&v.i, sizeof(v.i), 0);
	ClientSocket_send((char *)&key->type, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_sendDeleteKeyCommand(const char* trackName, int row)
{
	uint32_t track;
	unsigned char cmd = DELETE_KEY;
	int track_id = findTrack(trackName);

	if (!ClientSocket_connected() || track_id == -1)
		return;

	track = htonl((uint32_t)track_id);
	row = htonl(row);

	ClientSocket_send((char *)&cmd, 1, 0);
	ClientSocket_send((char *)&track, sizeof(int), 0);
	ClientSocket_send((char *)&row,   sizeof(int), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_sendSetRowCommand(int row)
{
	unsigned char cmd = SET_ROW;

	if (!ClientSocket_connected())
		return;

	row = htonl(row);
	ClientSocket_send((char *)&cmd, 1, 0);
	ClientSocket_send((char *)&row, sizeof(int), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_sendPauseCommand(bool pause)
{
	unsigned char cmd = PAUSE, flag = pause;

	if (!ClientSocket_connected())
		return;

	ClientSocket_send((char *)&cmd, 1, 0);
	ClientSocket_send((char *)&flag, 1, 0);

	s_paused = pause;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClientSocket_sendSaveCommand()
{
	unsigned char cmd = SAVE_TRACKS;

	if (!ClientSocket_connected())
		return;

	ClientSocket_send((char *)&cmd, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ClientSocket_isPaused()
{
	return s_paused;
}

