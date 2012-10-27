#include "RemoteConnection.h"
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
#include <arpa/inet.h>
#endif

#include "../../sync/base.h"
#include "../../sync/track.h"
#include "rlog.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

static int s_clientIndex;
int s_socket = INVALID_SOCKET;
int s_serverSocket = INVALID_SOCKET; 
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

void RemoteConnection_mapTrackName(const char* name)
{
	int count = s_nameLookup.count;

	if (findTrack(name))
		return;

	s_nameLookup.hashes[count] = quickHash(name);
	s_nameLookup.ids[count] = s_clientIndex++;
	s_nameLookup.count++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool setBlocking(int sock, bool blocking)
{
	u_long block = blocking ? 0 : 1;
#if defined(_WIN32)
	int error = ioctlsocket(sock, FIONBIO, &block);
#else
	int error = ioctl(sock, FIONBIO, &block);
#endif
	if (SOCKET_ERROR == error)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_createListner()
{
	s_serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (s_serverSocket == INVALID_SOCKET)
		return false;

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof sin);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(1338);

	if (-1 == bind(s_serverSocket, (struct sockaddr *)&sin, sizeof(sin)))
	{
		rlog(R_ERROR, "Unable to create server socket\n");
		return false;
	}

	while (listen(s_serverSocket, SOMAXCONN) == -1)
		;

	setBlocking(s_serverSocket, false);

	rlog(R_INFO, "Creaded listner\n");

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_connected()
{
	return INVALID_SOCKET != s_socket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static SOCKET clientConnect(SOCKET serverSocket, struct sockaddr_in* host)
{
	struct sockaddr_in hostTemp;
	char recievedGreeting[128];
	const char* expectedGreeting = CLIENT_GREET;
	const char* greeting = SERVER_GREET;
	unsigned int hostSize = sizeof(struct sockaddr_in);

	SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&hostTemp, &hostSize);

	if (INVALID_SOCKET == clientSocket) 
		return INVALID_SOCKET;

	recv(clientSocket, recievedGreeting, (int)strlen(expectedGreeting), 0);

	if (strncmp(expectedGreeting, recievedGreeting, strlen(expectedGreeting)) != 0)
	{
		closesocket(clientSocket);
		return INVALID_SOCKET;
	}

	send(clientSocket, greeting, (int)strlen(greeting), 0);

	if (NULL != host) 
		*host = hostTemp;

	return clientSocket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_updateListner()
{
	fd_set fds;
	struct timeval timeout;
	SOCKET clientSocket;
	struct sockaddr_in client;

	if (RemoteConnection_connected())
		return;

	rlog(R_INFO, "%d\n", s_socket);

	FD_ZERO(&fds);
	FD_SET(s_serverSocket, &fds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	// look for new clients
	
	//if (select(0, &fds, NULL, NULL, &timeout) > 0)
	{
		clientSocket = clientConnect(s_serverSocket, &client);

		if (INVALID_SOCKET != clientSocket)
		{
			rlog(R_INFO, "Connected to %s\n", inet_ntoa(client.sin_addr));
			s_socket = clientSocket; 
			s_clientIndex = 0;
			RemoteConnection_sendPauseCommand(true);
			//RemoteConnection_sendSetRowCommand(trackView->getEditRow());
		}
		else 
		{
			//
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_disconnect()
{
#if defined(_WIN32)
	closesocket(s_socket);
#else
	close(s_socket);
#endif
	s_socket = INVALID_SOCKET;

	rlog(R_INFO, "disconnect!\n");

	memset(s_nameLookup.ids, -1, sizeof(int) * s_nameLookup.count);
	s_nameLookup.count = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_recv(char* buffer, size_t length, int flags)
{
	int ret;

	if (!RemoteConnection_connected())
		return false;

	if ((ret = recv(s_socket, buffer, (int)length, flags)) != (int)length)
	{
		//rlog(R_INFO, "%d %d\n", ret, length);
		//RemoteConnection_disconnect();
		//return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_send(const char* buffer, size_t length, int flags)
{
	int ret;

	if (!RemoteConnection_connected())
		return false;

	if ((ret = send(s_socket, buffer, (int)length, flags)) != (int)length)
	{
		RemoteConnection_disconnect();
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_pollRead()
{
	if (!RemoteConnection_connected())
		return false;

	return !!socket_poll(s_socket);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sendSetKeyCommandIndex(uint32_t index, const struct track_key* key)
{
	uint32_t track, row;
	uint8_t cmd = SET_KEY;
	int track_id;

	union {
		float f;
		uint32_t i;
	} v;

	track = htonl(track_id);
	row = htonl(key->row);

	v.f = key->value;
	v.i = htonl(v.i);

	assert(key->type < KEY_TYPE_COUNT);

	RemoteConnection_send((char *)&cmd, 1, 0);
	RemoteConnection_send((char *)&track, sizeof(track), 0);
	RemoteConnection_send((char *)&row, sizeof(row), 0);
	RemoteConnection_send((char *)&v.i, sizeof(v.i), 0);
	RemoteConnection_send((char *)&key->type, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSetKeyCommand(const char* trackName, const struct track_key* key)
{
	int track_id = findTrack(trackName);

	if (!RemoteConnection_connected() || track_id == -1)
		return;

	sendSetKeyCommandIndex((uint32_t)track_id, key);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendDeleteKeyCommand(const char* trackName, int row)
{
	uint32_t track;
	unsigned char cmd = DELETE_KEY;
	int track_id = findTrack(trackName);

	if (!RemoteConnection_connected() || track_id == -1)
		return;

	track = htonl((uint32_t)track_id);
	row = htonl(row);

	RemoteConnection_send((char *)&cmd, 1, 0);
	RemoteConnection_send((char *)&track, sizeof(int), 0);
	RemoteConnection_send((char *)&row,   sizeof(int), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSetRowCommand(int row)
{
	unsigned char cmd = SET_ROW;

	if (!RemoteConnection_connected())
		return;

	row = htonl(row);
	RemoteConnection_send((char *)&cmd, 1, 0);
	RemoteConnection_send((char *)&row, sizeof(int), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendPauseCommand(bool pause)
{
	unsigned char cmd = PAUSE, flag = pause;

	if (!RemoteConnection_connected())
		return;

	RemoteConnection_send((char *)&cmd, 1, 0);
	RemoteConnection_send((char *)&flag, 1, 0);

	s_paused = pause;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSaveCommand()
{
	unsigned char cmd = SAVE_TRACKS;

	if (!RemoteConnection_connected())
		return;

	RemoteConnection_send((char *)&cmd, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_isPaused()
{
	return s_paused;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendKeyFrames(const char* name, struct sync_track* track)
{
	int i, track_id = findTrack(name);

	if (!RemoteConnection_connected() || track_id == -1)
		return;

	for (i = 0; i < (int)track->num_keys; ++i)
		sendSetKeyCommandIndex((uint32_t)track_id, &track->keys[i]);
}

