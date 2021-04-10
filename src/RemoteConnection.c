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

#include "../external/rocket/lib/base.h"
#include "../external/rocket/lib/track.h"
#include "rlog.h"
#include <stdio.h>

#include <assert.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif


/* configure socket-stack */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define USE_GETADDRINFO
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <limits.h>
#elif defined(USE_AMITCP)
#include <sys/socket.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <netdb.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define select(n,r,w,e,t) WaitSelect(n,r,w,e,t,0)
#define closesocket(x) CloseSocket(x)
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket(x) close(x)
#endif

#define CLIENT_GREET "hello, synctracker!"
#define SERVER_GREET "hello, demo!"

enum {
	SET_KEY = 0,
	DELETE_KEY = 1,
	GET_TRACK = 2,
	SET_ROW = 3,
	PAUSE = 4,
	SAVE_TRACKS = 5
};

static inline int socket_poll(SOCKET socket)
{
	struct timeval to = { 0, 0 };
	fd_set fds;

	FD_ZERO(&fds);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
	FD_SET(socket, &fds);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	return select((int)socket + 1, &fds, NULL, NULL, &to) > 0;
}

static int s_clientIndex;
int s_socket = INVALID_SOCKET;
int s_serverSocket = INVALID_SOCKET; 
static bool s_paused = true;
static char s_connectionName[256];

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

	if (findTrack(name) != -1)
		return;

	s_nameLookup.hashes[count] = quickHash(name);
	s_nameLookup.ids[count] = s_clientIndex++;
	s_nameLookup.count++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_createListner()
{
	struct sockaddr_in sin;
	int yes = 1;

#if defined(_WIN32)
	 WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0),&wsaData) != 0)
		return false;
#endif

	s_serverSocket = (int)socket(AF_INET, SOCK_STREAM, 0);

	if (s_serverSocket < 0) 
		return false;

	memset(&sin, 0, sizeof sin);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(1338);

	if (setsockopt(s_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == -1)
	{
		perror("setsockopt");
		return false;
	}

	if (-1 == bind(s_serverSocket, (struct sockaddr *)&sin, sizeof(sin)))
	{
		perror("bind");
		rlog(R_ERROR, "Unable to bind server socket\n");
		return false;
	}

	while (listen(s_serverSocket, SOMAXCONN) == -1)
		;

	//rlog(R_INFO, "Created listner\n");

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_connected()
{
	return INVALID_SOCKET != s_socket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int recv_all(SOCKET socket, char* buffer, size_t length, int flags)
{
	int remaining = (int)length, ret;

	while (remaining)
	{
		ret = recv(socket, buffer, remaining, flags);
		if (ret <= 0)
			return ret;
		remaining -= ret;
		buffer += ret;
	}
	return (int)length;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static SOCKET clientConnect(SOCKET serverSocket, struct sockaddr_in* host)
{
	struct sockaddr_in hostTemp;
	char recievedGreeting[128];
	const char* expectedGreeting = CLIENT_GREET;
	const char* greeting = SERVER_GREET;
	unsigned int hostSize = sizeof(struct sockaddr_in);

	SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&hostTemp, (socklen_t*)&hostSize);

	if (INVALID_SOCKET == clientSocket) 
		return INVALID_SOCKET;

	recv_all(clientSocket, recievedGreeting, strlen(expectedGreeting), 0);

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

void RemoteConnection_updateListner(int currentRow)
{
	struct timeval timeout;
	struct sockaddr_in client;
	SOCKET clientSocket = INVALID_SOCKET;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(s_serverSocket, &fds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	
	if (RemoteConnection_connected())
		return;

	// look for new clients
	
	if (select(s_serverSocket + 1, &fds, NULL, NULL, &timeout) > 0)
	{
		clientSocket = clientConnect(s_serverSocket, &client);

		if (INVALID_SOCKET != clientSocket)
		{
			snprintf(s_connectionName, sizeof(s_connectionName), "Connected to %s", inet_ntoa(client.sin_addr));
			//rlog(R_INFO, "%s\n", s_connectionName); 
			s_socket = (int)clientSocket; 
			s_clientIndex = 0;
			RemoteConnection_sendPauseCommand(true);
			RemoteConnection_sendSetRowCommand(currentRow);
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

	s_paused = true;

	memset(s_nameLookup.ids, -1, sizeof(int) * s_nameLookup.count);
	s_nameLookup.count = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int RemoteConnection_recv(char* buffer, size_t length, int flags)
{
	int ret;

	if (!RemoteConnection_connected())
		return false;

	ret = recv_all(s_socket, buffer, length, flags);

	if (ret != length)
	{
		RemoteConnection_disconnect();
		return false;
	}

	return ret;
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

	union {
		float f;
		uint32_t i;
	} v;

	track = htonl(index);
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

	//printf("rom %d\n", row);

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

void RemoteConnection_getConnectionStatus(char** status)
{
	if (!RemoteConnection_connected())
	{
		*status = "Not Connected";
		return;
	}
	
	*status = s_connectionName;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendKeyFrames(const char* name, struct sync_track* track)
{
	int i, track_id = findTrack(name);

	if (!RemoteConnection_connected() || track_id == -1)
		return;

	for (i = 0; i < track->num_keys; ++i)
		sendSetKeyCommandIndex((uint32_t)track_id, &track->keys[i]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_close()
{
	if (RemoteConnection_connected())
	{
		//rlog(R_INFO, "closing client socket %d\n", s_socket);
		closesocket(s_socket);
		s_socket = INVALID_SOCKET;
	}

	//rlog(R_INFO, "closing socket %d\n", s_serverSocket);

	closesocket(s_serverSocket);
	s_serverSocket = INVALID_SOCKET;
}
