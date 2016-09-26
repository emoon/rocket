#ifndef RENDERAUDIO_H
#define RENDERAUDIO_H

struct TrackData;

void RenderAudio_update(struct TrackData* trackData, int xPos, int yPos, int rowCount, int rowOffset, int rowSpacing, int endY);
void RenderAudio_render(int xPos, int yPos, int endY);

#endif
