#pragma once

#include "Types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrackData* trackData;
struct TrackViewInfo;

typedef struct TrackViewInfo
{
	int scrollPosY;
	int scrollPosX;
	int windowSizeX;
	int windowSizeY;
	int rowPos;
	int startTrack;
	int startPixel;
	int smallFontId;
	int selectStartTrack;
	int selectStopTrack;
	int selectStartRow;
	int selectStopRow;

} TrackViewInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init();
bool TrackView_render(TrackViewInfo* viewInfo, struct TrackData* trackData);
int TrackView_getWidth(TrackViewInfo* viewInfo, struct TrackData* trackData);
int TrackView_getScrolledTrack(struct TrackViewInfo* viewInfo, struct TrackData* trackData, int activeTrack, int posX);
int TrackView_getStartOffset();
int TrackView_getTracksOffset(struct TrackViewInfo* viewInfo, struct TrackData* trackData, int prevTrack, int nextTrack);
bool TrackView_isSelectedTrackVisible(struct TrackViewInfo* viewInfo, struct TrackData* trackData, int track);

