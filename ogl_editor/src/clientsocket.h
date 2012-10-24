#pragma once

#include <Types.h>

struct track_key;

bool ClientSocket_isPaused();
bool ClientSocket_connected();
void ClientSocket_disconnect();
bool ClientSocket_recv(char* buffer, size_t length, int flags);
bool ClientSocket_send(const char* buffer, size_t length, int flags);
bool ClientSocket_pollRead();
void ClientSocket_sendSetKeyCommand(const char* trackName, const struct track_key* key);
void ClientSocket_sendDeleteKeyCommand(const char* trackName, int row);
void ClientSocket_sendSetRowCommand(int row);
void ClientSocket_sendPauseCommand(bool pause);
void ClientSocket_sendSaveCommand();

