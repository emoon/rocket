#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrackData* trackData;

typedef struct TrackViewInfo
{
	int scrollPosY;
	int scrollPosX;
	int windowSizeX;
	int windowSizeY;
	int minRow;
	int rowPos;
	int maxRow;
	int smallFontId;
	int selectStartTrack;
	int selectStopTrack;
	int selectStartRow;
	int selectStopRow;

} TrackViewInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrackView_init();
void TrackView_render(const TrackViewInfo* viewInfo, struct TrackData* trackData);

