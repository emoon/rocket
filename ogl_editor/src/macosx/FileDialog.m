#include "../Dialog.h"
#import <Cocoa/Cocoa.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog_open(char* dest)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSOpenPanel* open = [NSOpenPanel openPanel];
	[open setAllowsMultipleSelection:NO];

	int result = [open runModal];

	if (result != NSOKButton)
		return false;

	// Grab the first file

	NSArray* selectedFiles = [open URLs];
	NSURL* url = [selectedFiles objectAtIndex:0];
	const char* temp = [[url path] UTF8String];

	strcpy(dest, temp);

	[pool drain];

	return true;
}


