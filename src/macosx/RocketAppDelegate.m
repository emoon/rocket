#import "RocketAppDelegate.h"
#include "../args.h"
#include "../loadsave.h"
#include "../Editor.h"
#include "../RemoteConnection.h"
#include "rlog.h"

void Window_populateRecentList(char** files);
void Window_buildMenu(void);

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

	// Check for rocket file argument and load it
	NSArray* arguments = [[NSProcessInfo processInfo] arguments];
	NSUInteger argCount = [arguments count];

	if (argCount > 1) {
		// Convert NSArray to C array for Args_parse
		const char** argv = malloc(argCount * sizeof(char*));
		for (NSUInteger i = 0; i < argCount; i++) {
			argv[i] = [[arguments objectAtIndex:i] UTF8String];
		}

		Args args;
		Args_parse(&args, (int)argCount, argv);

		if (args.loadFile) {
			if (LoadSave_loadRocketXML(args.loadFile, Editor_getTrackData())) {
				Editor_setLoadedFilename(args.loadFile);
			} else {
				NSAlert* alert = [[NSAlert alloc] init];
				[alert setMessageText:@"Failed to load rocket file"];
				[alert setInformativeText:[NSString stringWithFormat:@"Could not load file: %s", args.loadFile]];
				[alert setAlertStyle:NSWarningAlertStyle];
				[alert runModal];
				[alert release];
			}
		}

		free(argv);
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
	RemoteConnections_close();
	[stringArray release];
}

@end
