/* Copyright (C) 2007-2008 Erik Faye-Lund and Egbert Teeselink
 * For conditions of distribution and use, see copyright notice in COPYING
 */

#pragma once

#include "../sync/base.h"
#include <stack>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <cassert>

class NetworkSocket {
public:
	NetworkSocket() :
	    socket(INVALID_SOCKET)
	{
	}

	explicit NetworkSocket(SOCKET socket) :
	    socket(socket)
	{
	}

	bool connected() const
	{
		return INVALID_SOCKET != socket;
	}

	void disconnect()
	{
		closesocket(socket);
		socket = INVALID_SOCKET;
	}

	bool recv(char *buffer, size_t length, int flags)
	{
		if (!connected())
			return false;
		int ret = ::recv(socket, buffer, int(length), flags);
		if (ret != int(length)) {
			disconnect();
			return false;
		}
		return true;
	}

	bool send(const char *buffer, size_t length, int flags)
	{
		if (!connected())
			return false;
		int ret = ::send(socket, buffer, int(length), flags);
		if (ret != int(length)) {
			disconnect();
			return false;
		}
		return true;
	}

	bool pollRead()
	{
		if (!connected())
			return false;
		return !!socket_poll(socket);
	}

private:
	SOCKET socket;
};

struct KeyFrame {
	float value;
	enum Type {
		TYPE_STEP,
		TYPE_LINEAR,
		TYPE_SMOOTH,
		TYPE_RAMP,
		TYPE_COUNT
	} type;
};

struct SyncTrack {
	SyncTrack(const std::string &name) :
	    name(name)
	{
	}

	std::string name;
	std::map<int, KeyFrame> keys;
};

class SyncDocument {
public:
	SyncDocument() : clientPaused(true), rows(128), savePointDelta(0), savePointUnreachable(true)
	{
	}

	~SyncDocument();

	size_t createTrack(const std::string &name)
	{
		size_t index = tracks.size();
		tracks.push_back(new SyncTrack(name));
		trackOrder.push_back(index);
		return index;
	}

	void sendSetKeyCommand(uint32_t track, uint32_t row, const KeyFrame &key)
	{
		if (!clientSocket.connected())
			return;
		if (!clientRemap.count(track))
			return;
		track = htonl(clientRemap[track]);
		row = htonl(row);

		union {
			float f;
			uint32_t i;
		} v;
		v.f = key.value;
		v.i = htonl(v.i);

		assert(key.type < KeyFrame::TYPE_COUNT);

		unsigned char cmd = SET_KEY;
		clientSocket.send((char *)&cmd, 1, 0);
		clientSocket.send((char *)&track, sizeof(track), 0);
		clientSocket.send((char *)&row, sizeof(row), 0);
		clientSocket.send((char *)&v.i, sizeof(v.i), 0);
		clientSocket.send((char *)&key.type, 1, 0);
	}

	void sendDeleteKeyCommand(int track, int row)
	{
		if (!clientSocket.connected())
			return;
		if (!clientRemap.count(track))
			return;

		track = htonl(int(clientRemap[track]));
		row = htonl(row);

		unsigned char cmd = DELETE_KEY;
		clientSocket.send((char *)&cmd, 1, 0);
		clientSocket.send((char *)&track, sizeof(int), 0);
		clientSocket.send((char *)&row,   sizeof(int), 0);
	}

	void sendSetRowCommand(int row)
	{
		if (!clientSocket.connected())
			return;
		unsigned char cmd = SET_ROW;
		row = htonl(row);
		clientSocket.send((char *)&cmd, 1, 0);
		clientSocket.send((char *)&row, sizeof(int), 0);
	}

	void sendPauseCommand(bool pause)
	{
		if (!clientSocket.connected())
			return;
		unsigned char cmd = PAUSE;
		clientSocket.send((char *)&cmd, 1, 0);
		unsigned char flag = pause;
		clientSocket.send((char *)&flag, 1, 0);
		clientPaused = pause;
	}
	
	void sendSaveCommand()
	{
		if (!clientSocket.connected())
			return;
		unsigned char cmd = SAVE_TRACKS;
		clientSocket.send((char *)&cmd, 1, 0);
	}

	class Command {
	public:
		virtual ~Command()
		{
		}
		virtual void exec(SyncDocument *data) = 0;
		virtual void undo(SyncDocument *data) = 0;
	};

	class InsertCommand : public Command {
	public:
		InsertCommand(int track, int row, const KeyFrame &key) :
		    track(track),
		    row(row),
		    key(key)
		{
		}

		~InsertCommand()
		{
		}

		void exec(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(!t->keys.count(row));
			t->keys[row] = key;
			data->sendSetKeyCommand(track, row, key); // update clients
		}

		void undo(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(t->keys.count(row));
			t->keys.erase(row);
			data->sendDeleteKeyCommand(track, row); // update clients
		}

	private:
		int track, row;
		KeyFrame key;
	};
	
	class DeleteCommand : public Command {
	public:
		DeleteCommand(int track, int row) :
		    track(track), row(row)
		{
		}

		~DeleteCommand()
		{
		}

		void exec(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(t->keys.count(row));
			oldKey = t->keys[row];
			t->keys.erase(row);
			data->sendDeleteKeyCommand(track, row); // update clients
		}

		void undo(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(!t->keys.count(row));
			t->keys[row] = oldKey;
			data->sendSetKeyCommand(track, row, oldKey); // update clients
		}

	private:
		int track, row;
		KeyFrame oldKey;
	};

	
	class EditCommand : public Command {
	public:
		EditCommand(int track, int row, const KeyFrame &key) :
		    track(track),
		    row(row),
		    key(key)
		{
		}

		~EditCommand()
		{
		}

		void exec(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(t->keys.count(row));
			oldKey = t->keys[row];
			t->keys[row] = key;
			data->sendSetKeyCommand(track, row, key); // update clients
		}

		void undo(SyncDocument *data)
		{
			SyncTrack *t = data->tracks[track];
			assert(t->keys.count(row));
			t->keys[row] = oldKey;
			data->sendSetKeyCommand(track, row, oldKey); // update clients
		}

	private:
		int track, row;
		KeyFrame oldKey, key;
	};

	class MultiCommand : public Command {
	public:
		~MultiCommand()
		{
			std::list<Command*>::iterator it;
			for (it = commands.begin(); it != commands.end(); ++it)
				delete *it;
			commands.clear();
		}

		void addCommand(Command *cmd)
		{
			commands.push_back(cmd);
		}

		size_t getSize() const
		{
			return commands.size();
		}

		void exec(SyncDocument *data)
		{
			std::list<Command*>::iterator it;
			for (it = commands.begin(); it != commands.end(); ++it)
				(*it)->exec(data);
		}

		void undo(SyncDocument *data)
		{
			std::list<Command*>::reverse_iterator it;
			for (it = commands.rbegin(); it != commands.rend(); ++it)
				(*it)->undo(data);
		}

	private:
		std::list<Command*> commands;
	};

	void exec(Command *cmd)
	{
		undoStack.push(cmd);
		cmd->exec(this);
		clearRedoStack();

		if (savePointDelta < 0)
			savePointUnreachable = true;
		savePointDelta++;
	}
	
	bool undo()
	{
		if (!undoStack.size())
			return false;

		Command *cmd = undoStack.top();
		undoStack.pop();

		redoStack.push(cmd);
		cmd->undo(this);
		savePointDelta--;

		return true;
	}
	
	bool redo()
	{
		if (!redoStack.size())
			return false;

		Command *cmd = redoStack.top();
		redoStack.pop();

		undoStack.push(cmd);
		cmd->exec(this);

		savePointDelta++;

		return true;
	}
	
	void clearUndoStack()
	{
		while (!undoStack.empty())
		{
			Command *cmd = undoStack.top();
			undoStack.pop();
			delete cmd;
		}
	}
	
	void clearRedoStack()
	{
		while (!redoStack.empty())
		{
			Command *cmd = redoStack.top();
			redoStack.pop();
			delete cmd;
		}
	}
	
	Command *getSetKeyFrameCommand(int track, int row, const KeyFrame &key)
	{
		SyncTrack *t = tracks[track];
		return t->keys.count(row) ?
		    (Command *)new EditCommand(track, row, key) :
		    (Command *)new InsertCommand(track, row, key);
	}

	size_t getTrackOrderCount() const
	{
		return trackOrder.size();
	}
	
	size_t getTrackIndexFromPos(size_t track) const
	{
		assert(track < trackOrder.size());
		return trackOrder[track];
	}

	void swapTrackOrder(size_t t1, size_t t2)
	{
		assert(t1 < trackOrder.size());
		assert(t2 < trackOrder.size());
		std::swap(trackOrder[t1], trackOrder[t2]);
	}
	
	bool load(const std::wstring &fileName);
	bool save(const std::wstring &fileName);

	bool modified()
	{
		if (savePointUnreachable) return true;
		return 0 != savePointDelta;
	}
	
	NetworkSocket clientSocket;
	std::map<size_t, size_t> clientRemap;
	bool clientPaused;

	size_t getRows() const { return rows; }
	void setRows(size_t rows) { this->rows = rows; }

	size_t getTrackCount() const { return tracks.size(); }
	SyncTrack *getTrack(size_t idx) { return tracks[idx]; }
	const SyncTrack *getTrack(size_t idx) const { return tracks[idx]; }

private:
	std::vector<SyncTrack *> tracks;
	std::vector<size_t> trackOrder;
	size_t rows;
	
	// undo / redo functionality
	std::stack<Command*> undoStack;
	std::stack<Command*> redoStack;
	int savePointDelta;        // how many undos must be done to get to the last saved state
	bool savePointUnreachable; // is the save-point reachable?
};
