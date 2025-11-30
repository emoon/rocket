#include "RemoteConnection.h"
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4496)
#include <winsock2.h>
#pragma warning(default : 4496)
#endif

#if defined(_WIN32)
#include <Ws2tcpip.h>
#include <winsock2.h>
#endif

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include "../external/rocket/lib/base.h"
#include "../external/rocket/lib/track.h"
#include "rlog.h"

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
#include <limits.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(USE_AMITCP)
#include <netdb.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define select(n, r, w, e, t) WaitSelect(n, r, w, e, t, 0)
#define closesocket(x) CloseSocket(x)
#else
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket(x) close(x)
#endif

#define CLIENT_GREET "hello, synctracker!"
#define SERVER_GREET "hello, demo!"

enum { SET_KEY = 0, DELETE_KEY = 1, GET_TRACK = 2, SET_ROW = 3, PAUSE = 4, SAVE_TRACKS = 5 };

#define MAX_CONNECTIONS 10

typedef struct RemoteConnection {
    int s_socket;
    char s_connectionName[256];
} RemoteConnection;

static int s_trackMapIndex;
static RemoteConnection s_connections[MAX_CONNECTIONS];
RemoteConnection* s_demo_connection = NULL; /* FIXME: icky global mostly because Editor.c open-codes cmd parsing, when that should really be in here */
static int s_num_connections;
static int s_serverSocket = INVALID_SOCKET;
static bool s_paused = true;
static char s_connectionStatus[1024];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum {
    MAX_NAME_HASHES = 8192,  // Should be enough with 8k tracks/channels :)
};

struct NameLookup {
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

static uint32_t quickHash(const char* string) {
    uint32_t c;
    uint32_t hash = 0;

    const uint8_t* str = (const uint8_t*)string;

    while ((c = *str++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash & 0x7FFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int findTrack(const char* name) {
    uint32_t i, count, hash = quickHash(name);

    for (i = 0, count = s_nameLookup.count; i < count; ++i) {
        if (hash == s_nameLookup.hashes[i])
            return s_nameLookup.ids[i];
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnections_mapTrackName(const char* name) {
    int count = s_nameLookup.count;

    if (findTrack(name) != -1)
        return;

    s_nameLookup.hashes[count] = quickHash(name);
    s_nameLookup.ids[count] = s_trackMapIndex++;
    s_nameLookup.count++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnections_createListner(void) {
    struct sockaddr_in sin;
    int yes = 1, i;

#if defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
        return false;
#endif

    s_serverSocket = (int)socket(AF_INET, SOCK_STREAM, 0);

    if (s_serverSocket < 0)
        return false;

    memset(&sin, 0, sizeof sin);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(1338);

    if (setsockopt(s_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return false;
    }

    if (-1 == bind(s_serverSocket, (struct sockaddr*)&sin, sizeof(sin))) {
        perror("bind");
        rlog(R_ERROR, "Unable to bind server socket\n");
        return false;
    }

    while (listen(s_serverSocket, SOMAXCONN) == -1)
        ;

    for (i = 0; i < MAX_CONNECTIONS; i++)
        s_connections[i].s_socket = INVALID_SOCKET;

    s_num_connections = 0;
    // rlog(R_INFO, "Created listner\n");

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int recv_all(SOCKET socket, char* buffer, size_t length, int flags) {
    int remaining = (int)length, ret;

    while (remaining) {
        ret = recv(socket, buffer, remaining, flags);
        if (ret <= 0)
            return ret;
        remaining -= ret;
        buffer += ret;
    }
    return (int)length;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static SOCKET clientConnect(SOCKET serverSocket, struct sockaddr_in* host) {
    struct sockaddr_in hostTemp;
    char recievedGreeting[128];
    const char* expectedGreeting = CLIENT_GREET;
    const char* greeting = SERVER_GREET;
    unsigned int hostSize = sizeof(struct sockaddr_in);
    int yes = 1;

    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&hostTemp, (socklen_t*)&hostSize);

    if (INVALID_SOCKET == clientSocket)
        return INVALID_SOCKET;

    recv_all(clientSocket, recievedGreeting, strlen(expectedGreeting), 0);

    if (strncmp(expectedGreeting, recievedGreeting, strlen(expectedGreeting)) != 0) {
        closesocket(clientSocket);
        return INVALID_SOCKET;
    }

    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) == -1)
        rlog(R_INFO, "failed to set TCP_NODELAY on client socket\n");

    send(clientSocket, greeting, (int)strlen(greeting), 0);

    if (NULL != host)
        *host = hostTemp;

    return clientSocket;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnections_updateListner(int currentRow) {
    struct timeval timeout;
    struct sockaddr_in client;
    SOCKET clientSocket = INVALID_SOCKET;
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(s_serverSocket, &fds);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // look for new clients

    if (select(s_serverSocket + 1, &fds, NULL, NULL, &timeout) > 0) {
        clientSocket = clientConnect(s_serverSocket, &client);

        if (INVALID_SOCKET != clientSocket) {
            RemoteConnection* conn = NULL;
            int i;

            if (s_num_connections >= MAX_CONNECTIONS) {
                rlog(R_INFO, "MAX_CONNECTIONS exceeded!\n");
                return;
            }

            for (i = 0; i < MAX_CONNECTIONS; i++) {
                if (s_connections[i].s_socket == INVALID_SOCKET) {
                    conn = &s_connections[i];
                    break;
                }
            }

            assert (conn && i < MAX_CONNECTIONS);

            conn->s_socket = (int)clientSocket;
            snprintf(conn->s_connectionName, sizeof(conn->s_connectionName), "%s", inet_ntoa(client.sin_addr));
            // rlog(R_INFO, "Connected to %s\n", conn->s_connectionName);
            RemoteConnection_sendPauseCommand(conn, s_paused);
            RemoteConnection_sendSetRowCommand(conn, currentRow);
            s_paused = true;

            s_num_connections++;
        } else {
            //
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_connected(RemoteConnection *conn) {
    return !!(conn && INVALID_SOCKET != conn->s_socket);
}

bool RemoteConnections_connected(void) {
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (RemoteConnection_connected(&s_connections[i]))
            return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RemoteConnection_disconnect(RemoteConnection *conn) {
    if (!RemoteConnection_connected(conn))
        return;

    rlog(R_INFO, "disconnecting!\n");

#if defined(_WIN32)
    closesocket(conn->s_socket);
#else
    close(conn->s_socket);
#endif
    conn->s_socket = INVALID_SOCKET;

    if (s_demo_connection == conn) {
        rlog(R_INFO, "disconnected demo!\n");

        s_demo_connection = NULL;
        s_trackMapIndex = 0;
        memset(s_nameLookup.ids, -1, sizeof(int) * s_nameLookup.count);
        s_nameLookup.count = 0;
    }

    s_num_connections--;
}

void RemoteConnections_disconnect(void) {
    int i;

    rlog(R_INFO, "disconnect everyone!\n");

    for (i = 0; i < MAX_CONNECTIONS; i++)
            RemoteConnection_disconnect(&s_connections[i]);

    s_num_connections = 0;

    s_paused = true;

    memset(s_nameLookup.ids, -1, sizeof(int) * s_nameLookup.count);
    s_nameLookup.count = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RemoteConnection_recv(RemoteConnection *conn, char* buffer, size_t length, int flags) {
    int ret;

    if (!RemoteConnection_connected(conn))
        return false;

    ret = recv_all(conn->s_socket, buffer, length, flags);

    if (ret != length) {
        RemoteConnection_disconnect(conn);
        return false;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnection_send(RemoteConnection *conn, const char* buffer, size_t length, int flags) {
    int ret;

    if (!RemoteConnection_connected(conn))
        return false;

    if ((ret = send(conn->s_socket, buffer, (int)length, flags)) != (int)length) {
        RemoteConnection_disconnect(conn);
        return false;
    }

    return true;
}

bool RemoteConnections_send(const char* buffer, size_t length, int flags) {
    bool ret = false;
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
            if (RemoteConnection_send(&s_connections[i], buffer, length, flags))
                ret = true;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* finds first readable conection in all connections, if any are ready return in *res_conn and returns true */
bool RemoteConnections_pollRead(RemoteConnection **res_conn) {
    int             i, maxfd = -1;
    struct timeval  to = {0, 0};
    fd_set          fds;

    FD_ZERO(&fds);
    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (s_connections[i].s_socket != INVALID_SOCKET) {
                FD_SET(s_connections[i].s_socket, &fds);
                if (s_connections[i].s_socket > maxfd)
                    maxfd = s_connections[i].s_socket;
        }
    }

    if (maxfd == -1)
        return false;

    if (select((int)maxfd + 1, &fds, NULL, NULL, &to) > 0) {
        for (i = 0; i < MAX_CONNECTIONS; i++) {
            if (s_connections[i].s_socket == INVALID_SOCKET)
                continue;

            if (FD_ISSET(s_connections[i].s_socket, &fds)) {
                if (res_conn)
                    *res_conn = &s_connections[i];

                return true;
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sendSetKeyCommandIndex(RemoteConnection *conn, uint32_t index, const struct track_key* key) {
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

    RemoteConnection_send(conn, (char*)&cmd, 1, 0);
    RemoteConnection_send(conn, (char*)&track, sizeof(track), 0);
    RemoteConnection_send(conn, (char*)&row, sizeof(row), 0);
    RemoteConnection_send(conn, (char*)&v.i, sizeof(v.i), 0);
    RemoteConnection_send(conn, (char*)&key->type, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSetKeyCommand(RemoteConnection *conn, const char* trackName, const struct track_key* key) {
    int track_id = findTrack(trackName);

    if (!RemoteConnection_connected(conn) || track_id == -1)
        return;

    sendSetKeyCommandIndex(conn, (uint32_t)track_id, key);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendDeleteKeyCommand(RemoteConnection *conn, const char* trackName, int row) {
    uint32_t track;
    unsigned char cmd = DELETE_KEY;
    int track_id = findTrack(trackName);

    if (!RemoteConnection_connected(conn) || track_id == -1)
        return;

    track = htonl((uint32_t)track_id);
    row = htonl(row);

    RemoteConnection_send(conn, (char*)&cmd, 1, 0);
    RemoteConnection_send(conn, (char*)&track, sizeof(int), 0);
    RemoteConnection_send(conn, (char*)&row, sizeof(int), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSetRowCommand(RemoteConnection *conn, int row) {
    unsigned char cmd = SET_ROW;

    if (!RemoteConnection_connected(conn))
        return;

    // printf("rom %d\n", row);

    row = htonl(row);
    RemoteConnection_send(conn, (char*)&cmd, 1, 0);
    RemoteConnection_send(conn, (char*)&row, sizeof(int), 0);
}

void RemoteConnections_sendSetRowCommand(int row, RemoteConnection* skip) {
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (&s_connections[i] == skip)
            continue;

        RemoteConnection_sendSetRowCommand(&s_connections[i], row);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendPauseCommand(RemoteConnection *conn, bool pause) {
    unsigned char cmd = PAUSE, flag = pause;

    if (!RemoteConnection_connected(conn))
        return;

    RemoteConnection_send(conn, (char*)&cmd, 1, 0);
    RemoteConnection_send(conn, (char*)&flag, 1, 0);
}

void RemoteConnections_sendPauseCommand(bool pause) {
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++)
        RemoteConnection_sendPauseCommand(&s_connections[i], pause);

    s_paused = pause;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendSaveCommand(RemoteConnection *conn) {
    unsigned char cmd = SAVE_TRACKS;

    if (!RemoteConnection_connected(conn))
        return;

    RemoteConnection_send(conn, (char*)&cmd, 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RemoteConnections_isPaused(void) {
    return s_paused;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnections_getConnectionStatus(char** status) {
    if (!RemoteConnections_connected()) {
        *status = "Disconnected";
        return;
    }

    if (!s_demo_connection || s_demo_connection->s_socket == INVALID_SOCKET) {
        snprintf(s_connectionStatus, sizeof(s_connectionStatus), "No Demo Connected, +%i Client%s", s_num_connections, s_num_connections > 1 ? "s" : "");
    } else if (s_num_connections == 1) {
        snprintf(s_connectionStatus, sizeof(s_connectionStatus), "Demo @ %s", s_demo_connection->s_connectionName);
    } else {
        snprintf(s_connectionStatus, sizeof(s_connectionStatus), "Demo @ %s, +%i Client%s", s_demo_connection->s_connectionName, s_num_connections - 1, s_num_connections > 2 ? "s" : "");
    }

    *status = s_connectionStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnection_sendKeyFrames(RemoteConnection *conn, const char* name, struct sync_track* track) {
    int i, track_id = findTrack(name);

    if (!RemoteConnection_connected(conn) || track_id == -1)
        return;

    for (i = 0; i < track->num_keys; ++i)
        sendSetKeyCommandIndex(conn, (uint32_t)track_id, &track->keys[i]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RemoteConnections_close(void) {
    RemoteConnections_disconnect();

    // rlog(R_INFO, "closing socket %d\n", s_serverSocket);

    closesocket(s_serverSocket);
    s_serverSocket = INVALID_SOCKET;
}
