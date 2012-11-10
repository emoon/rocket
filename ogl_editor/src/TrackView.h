#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrackData* trackData;

typedef struct TrackViewInfo
{
	int scrollPosY;
	int scrollPosX;
	int windowSizeX;
	int windowSizeY;
	int rowPos;
	int startRow;
	int endRow;
	int startTrack;
	int smallFontId;
	int selectStartTrack;
	int selectStopTrack;
	int selectStartRow;
	int selectStopRow;

} TrackViewInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init();
void TrackView_render(TrackViewInfo* viewInfo, struct TrackData* trackData);

