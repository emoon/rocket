#pragma once

#include <Types.h>

struct track_key;
struct sync_track;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Listen for incoming connections

bool RemoteConnection_createListner();
void RemoteConnection_updateListner();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Talk with the demo stuff

bool RemoteConnection_isPaused();
bool RemoteConnection_connected();
void RemoteConnection_disconnect();
bool RemoteConnection_recv(char* buffer, size_t length, int flags);
bool RemoteConnection_send(const char* buffer, size_t length, int flags);
bool RemoteConnection_pollRead();

void RemoteConnection_sendSetKeyCommand(const char* trackName, const struct track_key* key);
void RemoteConnection_sendDeleteKeyCommand(const char* trackName, int row);
void RemoteConnection_sendSetRowCommand(int row);
void RemoteConnection_sendPauseCommand(bool pause);
void RemoteConnection_sendSaveCommand();

void RemoteConnection_sendKeyFrames(const char* name, struct sync_track* track);
void RemoteConnection_mapTrackName(const char* name, int index);

