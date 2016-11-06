#ifndef _OGLEDITOR_COMMANDS_H_
#define _OGLEDITOR_COMMANDS_H_

struct sync_track;
struct track_key;
struct TrackData;
struct Track;
struct TrackViewInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_init(struct sync_track** syncTracks, struct TrackData* trackData);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Commands_needsSave();

void Commands_undo();
void Commands_redo();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Commands_deleteKey(int track, int row);
void Commands_addKey(int track, struct track_key* key);
void Commands_addOrUpdateKey(int track, struct track_key* key);
void Commands_toggleMute(struct Track* track, struct sync_track* syncTrack, int row);
void Commands_toggleBookmark(struct TrackData* trackData, int row);
void Commands_clearBookmarks(struct TrackData* trackData);
void Commands_toggleLoopmark(struct TrackData* trackData, int row);
void Commands_clearLoopmarks(struct TrackData* trackData);
void Commands_updateKey(int track, struct track_key* key); 
void Commands_setSelection(struct TrackViewInfo* viewInfo, int startTrack, int endTrack, int startRow, int endRow);
void Commands_beginMulti(const char* name); // Used (for example) when changing many value at the same time
void Commands_endMulti();
int Commands_undoCount();

#endif
