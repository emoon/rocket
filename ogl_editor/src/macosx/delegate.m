#import "delegate.h"
#include "../Editor.h"
#include "../RemoteConnection.h"
#include "rlog.h"

void Window_populateRecentList(char** files);

@implementation MinimalAppAppDelegate

@synthesize window;
@synthesize button;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
	char** recent_list = Editor_getRecentFiles();

	NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];

	if (prefs)
	{
		NSArray* stringArray = [prefs objectForKey:@"recentFiles"];

		for (int i = 0; i < 4; ++i)
		{
			NSString* name = [stringArray objectAtIndex:i];
			const char* filename = [name cStringUsingEncoding:NSASCIIStringEncoding];
			if (filename)
				strcpy(recent_list[i], filename);
		}
	}

	Window_populateRecentList(recent_list);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction) buttonClicked:(id)sender 
{
	Editor_menuEvent((int)((NSButton*)sender).tag);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	int i;
	NSMutableArray* stringArray;
	char** recent_list = Editor_getRecentFiles();
	stringArray = [[NSMutableArray alloc] init];

	for (i = 0; i < 4; ++i)
		[stringArray addObject:[NSString stringWithUTF8String: recent_list[i]]];

	[[NSUserDefaults standardUserDefaults] setObject:stringArray forKey:@"recentFiles"];
	[[NSUserDefaults standardUserDefaults] synchronize];

	Editor_destroy();
	RemoteConnection_close();
}

@end
