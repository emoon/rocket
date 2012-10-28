#pragma once

struct TrackData;

int LoadSave_loadRocketXML(const char* path, struct TrackData* trackData);
int LoadSave_loadRocketXMLDialog(struct TrackData* trackData);
int LoadSave_saveRocketXML(const char* path);
int LoadSave_saveRocketXMLDialog();

