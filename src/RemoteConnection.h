#pragma once

#include <emgui/Types.h>

struct track_key;
struct sync_track;
typedef struct RemoteConnection RemoteConnection;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Listen for incoming connections

bool RemoteConnections_createListner();
void RemoteConnections_updateListner(int currentRow);
void RemoteConnections_close();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Talk with _all_ the remote connections
bool RemoteConnections_isPaused();
bool RemoteConnections_connected();
void RemoteConnections_disconnect();

bool RemoteConnections_send(const char* buffer, size_t length, int flags);
bool RemoteConnections_pollRead(RemoteConnection **res_conn);

void RemoteConnections_sendSetRowCommand(int row, RemoteConnection* skip);
void RemoteConnections_sendPauseCommand(bool pause);

void RemoteConnections_mapTrackName(const char* name);

void RemoteConnections_getConnectionStatus(char** status);


/* these only operate on a single connection */
bool RemoteConnection_connected(RemoteConnection *conn);
void RemoteConnection_disconnect(RemoteConnection *conn);
int RemoteConnection_recv(RemoteConnection *conn, char* buffer, size_t length, int flags);
bool RemoteConnection_send(RemoteConnection *conn, const char* buffer, size_t length, int flags);
void RemoteConnection_sendSetKeyCommand(RemoteConnection *conn, const char* trackName, const struct track_key* key);
void RemoteConnection_sendDeleteKeyCommand(RemoteConnection *conn, const char* trackName, int row);
void RemoteConnection_sendSetRowCommand(RemoteConnection *conn, int row);
void RemoteConnection_sendPauseCommand(RemoteConnection *conn, bool pause);
void RemoteConnection_sendSaveCommand(RemoteConnection *conn);
void RemoteConnection_sendKeyFrames(RemoteConnection *conn, const char* name, struct sync_track* track);
