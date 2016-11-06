#import "RocketAppDelegate.h"
#include "../Editor.h"
#include "../RemoteConnection.h"
#include "rlog.h"

void Window_populateRecentList(char** files);
void Window_buildMenu();

@implementation RocketAppDelegate

@synthesize window;
@synthesize button;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	NSUInteger exitcode = NSTerminateNow;
	
	if (!Editor_needsSave())
		return exitcode;

	NSAlert *alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"Yes"];
	[alert addButtonWithTitle:@"No"];
	[alert addButtonWithTitle:@"Cancel"];
	[alert setMessageText:@"Save before exit?"];
	[alert setInformativeText:@"Do you want save the work?"];
	[alert setAlertStyle:NSWarningAlertStyle];

	NSModalResponse ret = [alert runModal];

	if (ret == NSAlertFirstButtonReturn)
	{
		if (!Editor_saveBeforeExit())
			exitcode = NSTerminateCancel;
	}

	if (ret == NSAlertThirdButtonReturn)
		exitcode = NSTerminateCancel;

	[alert release];

	return exitcode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
	char** recent_list = Editor_getRecentFiles();

	NSUserDefaults* prefs = [NSUserDefaults standardUserDefaults];

	for (int i = 0; i < 4; ++i)
		recent_list[i][0] = 0;

	if (prefs)
	{
		NSArray* stringArray = [prefs objectForKey:@"recentFiles"];
		int recentIndex = 0;

		for (int i = 0; i < 4; ++i)
		{
			NSString* name = [stringArray objectAtIndex:i];
			const char* filename = [name cStringUsingEncoding:NSUTF8StringEncoding];
			if (filename && filename[0] != 0)
				strcpy(recent_list[recentIndex++], filename);
		}
	}

	Window_buildMenu();
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
	[stringArray release];
}

@end
