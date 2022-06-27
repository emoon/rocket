#import "RocketView.h"
#include "../Editor.h"
#include "../rlog.h"
#include "../Menu.h"
#include <emgui/Emgui.h> 
#include <emgui/GFXBackend.h> 
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

NSOpenGLContext* g_context = nil;
NSWindow* g_window = nil;

void Window_setTitle(const char* title);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scan codes on Mac taken from http://boredzo.org/blog/archives/2007-05-22/virtual-key-codes

#define KEY_RETURN 36
#define KEY_TAB 48
#define KEY_DELETE 51
#define KEY_ESCAPE 53

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MyMenuItem : NSMenuItem 
{
}
- (BOOL)isHighlighted;
@end

@implementation MyMenuItem
- (BOOL)isHighlighted
{
	return NO;
}
@end

@implementation RocketView

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(void) updateEditor
{
	Editor_timedUpdate();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(NSRect)frame 
{
	self = [super initWithFrame:frame];
	if (self == nil)
		return nil;

	NSOpenGLPixelFormatAttribute attributes[] = {
			NSOpenGLPFADoubleBuffer,
			0
	};

	NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	oglContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
	[format release];
	[oglContext makeCurrentContext];

	g_context = oglContext;
	g_window = [self window];

	oglContext.view = self;
	
	EMGFXBackend_create();
	Editor_create();
	Editor_update();

	const float framerate = 60;
	const float frequency = 1.0f/framerate;
	[NSTimer scheduledTimerWithTimeInterval:frequency
									 target:self selector:@selector(updateEditor)
								   userInfo:nil repeats:YES];

	return self;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)lockFocus
{
	NSOpenGLContext* context = oglContext;

	[super lockFocus];

	if ([context view] != self) 
		[context setView:self];

	[context makeCurrentContext];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(NSRect)frameRect 
{
	[oglContext update];
	g_window = [self window];

	EMGFXBackend_updateViewPort((int)frameRect.size.width, (int)frameRect.size.height);
	Editor_setWindowSize((int)frameRect.size.width, (int)frameRect.size.height);
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int getModifierFlags(int flags)
{
	int specialKeys = 0;

	if (flags & NSShiftKeyMask)
		specialKeys |= EMGUI_KEY_SHIFT;

	if (flags & NSAlternateKeyMask)
		specialKeys |= EMGUI_KEY_ALT;

	if (flags & NSControlKeyMask)
		specialKeys |= EMGUI_KEY_CTRL;

	if (flags & NSCommandKeyMask)
		specialKeys |= EMGUI_KEY_COMMAND;

	return specialKeys;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyDown:(NSEvent *)theEvent 
{
	NSString* key = [theEvent charactersIgnoringModifiers];
	unichar keyChar = 0;
	if ([key length] == 0)
		return;

	keyChar = [key characterAtIndex:0];

	int keyCode = keyChar;
	int specialKeys = getModifierFlags([theEvent modifierFlags]);

	if ([theEvent modifierFlags] & NSNumericPadKeyMask) 
	{ 
		switch (keyChar)
		{
			case NSLeftArrowFunctionKey: keyCode = EMGUI_KEY_ARROW_LEFT; break;
			case NSRightArrowFunctionKey: keyCode = EMGUI_KEY_ARROW_RIGHT; break;
			case NSUpArrowFunctionKey: keyCode = EMGUI_KEY_ARROW_UP; break;
			case NSDownArrowFunctionKey: keyCode = EMGUI_KEY_ARROW_DOWN; break;
		}
	}
	else
	{
		switch ([theEvent keyCode])
		{
			case KEY_TAB : keyCode = EMGUI_KEY_TAB; break;
			case KEY_DELETE : keyCode = EMGUI_KEY_BACKSPACE; break;
			case KEY_RETURN : keyCode = EMGUI_KEY_ENTER; break;
			case KEY_ESCAPE : keyCode = EMGUI_KEY_ESC; break;
			case NSPageDownFunctionKey: keyCode = EMGUI_KEY_PAGE_DOWN; break;
			case NSPageUpFunctionKey: keyCode = EMGUI_KEY_PAGE_UP; break;
		}
	}

	Emgui_sendKeyinput(keyCode, specialKeys);

	if (!Editor_keyDown(keyCode, [theEvent keyCode], specialKeys))
		[super keyDown:theEvent];

	Editor_update();
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstResponder 
{
	return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(void) viewWillMoveToWindow:(NSWindow *)newWindow
{
	NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[self frame]
	                                                            options: (NSTrackingMouseMoved | NSTrackingActiveAlways)
	                                                              owner:self
	                                                           userInfo:nil];
	[self addTrackingArea:trackingArea];
	[trackingArea release];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseMoved:(NSEvent *)event
{
	NSWindow* window = [self window];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];
	NSRect adjustFrame = [NSWindow contentRectForFrameRect: originalFrame styleMask: NSTitledWindowMask];

	Emgui_setMousePos((int)location.x, (int)adjustFrame.size.height - (int)location.y);
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseDragged:(NSEvent *)event
{
	NSWindow* window = [self window];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];
	NSRect adjustFrame = [NSWindow contentRectForFrameRect: originalFrame styleMask: NSTitledWindowMask];

	Emgui_setMousePos((int)location.x, (int)adjustFrame.size.height - (int)location.y);
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)scrollWheel:(NSEvent *)theEvent
{
	float x = (float)[theEvent deltaX];
	float y = (float)[theEvent deltaY];
	int flags = getModifierFlags([theEvent modifierFlags]);

	//printf("%f %f %d\n", x, y, flags);
	Editor_scroll(-x, -y, flags);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseUp:(NSEvent *)event
{
	Emgui_setMouseLmb(0);
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseDown:(NSEvent *)event
{
	NSWindow *window = [self window];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];
	NSRect adjustFrame = [NSWindow contentRectForFrameRect: originalFrame styleMask: NSTitledWindowMask];

	Emgui_setMousePos((int)location.x, (int)adjustFrame.size.height - (int)location.y);
	Emgui_setMouseLmb(1);
	
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(BOOL) isOpaque
{
	return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFStringRef createStringForKey(CGKeyCode keyCode)
{
	TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
	CFDataRef layoutData = TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);

	if (!layoutData)
		return 0;

	const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);

	UInt32 keysDown = 0;
	UniChar chars[4];
	UniCharCount realLength;

	UCKeyTranslate( keyboardLayout,
					keyCode,
					kUCKeyActionDisplay,
					0,
					LMGetKbdType(),
					kUCKeyTranslateNoDeadKeysBit,
					&keysDown,
					sizeof(chars) / sizeof(chars[0]),
					&realLength,
					chars );
	CFRelease(currentKeyboard);

	return CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)onMenuPress:(id)sender 
{
	int id = (int)((NSButton*)sender).tag;
	Editor_menuEvent(id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static CGKeyCode s_characterToKeyCode[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x27, // '''
	0, 0, 0, 0,
	0x2b, // ','
	0x1b, // '-'
	0x2f, // '.'
	0x2c, // '/'
	0x1d, // '0'
	0x12, // '1'
	0x13, // '2'
	0x14, // '3'
	0x15, // '4'
	0x17, // '5'
	0x16, // '6'
	0x1a, // '7'
	0x1c, // '8'
	0x19, // '9'
	0,
	0x29, // ';'
	0,
	0x18, // '='
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x21, // '['
	0x2a, // '\'
	0x1e, // ']'
	0,
	0,
	0x32, // '`'
	0,
	0x0b, // 'b'
	0x08, // 'c'
	0x02, // 'd'
	0x0e, // 'e'
	0x03, // 'f'
	0x05, // 'g'
	0x04, // 'h'
	0x22, // 'i'
	0x26, // 'j'
	0x28, // 'k'
	0x25, // 'l'
	0x2e, // 'm'
	0x2d, // 'n'
	0x1f, // 'o'
	0x23, // 'p'
	0x0c, // 'q'
	0x0f, // 'r'
	0x01, // 's'
	0x11, // 't'
	0x20, // 'u'
	0x09, // 'v'
	0x0d, // 'w'
	0x07, // 'x'
	0x10, // 'y'
	0x06, // 'z'
	0, 0, 0, 0, 0,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NSString* convertKeyCodeToString(int key)
{
	if (key < 128)
	{
		// first try to translate it and if that doesn't work use it as is
		NSString* charName = (NSString*)createStringForKey(s_characterToKeyCode[key]);

		if (charName)
			return charName;

		return [NSString stringWithFormat:@"%c", (char)key]; 
	}
	else
	{
		switch (key)
		{
			case EMGUI_KEY_ARROW_UP: return [NSString stringWithFormat:@"%C", (uint16_t)0x2191];
			case EMGUI_KEY_ARROW_DOWN: return [NSString stringWithFormat:@"%C", (uint16_t)0x2193];
			case EMGUI_KEY_ARROW_LEFT: return [NSString stringWithFormat:@"%C", (uint16_t)0x2190];
			case EMGUI_KEY_ARROW_RIGHT: return [NSString stringWithFormat:@"%C", (uint16_t)0x2192];
			case EMGUI_KEY_ESC : return [NSString stringWithFormat:@"%C", (uint16_t)0x238b];
			case EMGUI_KEY_ENTER : return [NSString stringWithFormat:@"%C", (uint16_t)NSCarriageReturnCharacter];
			case EMGUI_KEY_SPACE : return @" "; 
			case EMGUI_KEY_BACKSPACE : return [NSString stringWithFormat:@"%C",(uint16_t)0x232b];
			case EMGUI_KEY_TAB : return [NSString stringWithFormat:@"%C",(uint16_t)0x21e4]; 
			case EMGUI_KEY_PAGE_UP : return [NSString stringWithFormat:@"%C",(uint16_t)0x21de]; 
			case EMGUI_KEY_PAGE_DOWN : return [NSString stringWithFormat:@"%C",(uint16_t)0x21df];
		}
	}

	return nil;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void buildSubMenu(NSMenu* menu, MenuDescriptor menuDesc[])
{
	MenuDescriptor* desc = &menuDesc[0];
	[menu removeAllItems];

	while (desc->name)
	{
		NSString* name = [NSString stringWithUTF8String: desc->name];

		if (desc->id == EDITOR_MENU_SEPARATOR)
		{
			[menu addItem:[NSMenuItem separatorItem]];
		}
		else if (desc->id == EDITOR_MENU_SUB_MENU)
		{
			MyMenuItem* newItem = [[MyMenuItem alloc] initWithTitle:name action:NULL keyEquivalent:@""];
			NSMenu* newMenu = [[NSMenu alloc] initWithTitle:name];
			[newItem setSubmenu:newMenu];
			[newMenu release];
			[menu addItem:newItem];
			[newItem release];
		}
		else
		{
			NSUInteger mask = 0;
			MyMenuItem* newItem = [[MyMenuItem alloc] initWithTitle:name action:@selector(onMenuPress:) keyEquivalent:@""];
			[newItem setTag:desc->id];

			if (desc->macMod & EMGUI_KEY_COMMAND)
				mask |= NSCommandKeyMask;
			if (desc->macMod & EMGUI_KEY_SHIFT)
				mask |= NSShiftKeyMask;
			if (desc->macMod & EMGUI_KEY_CTRL)
				mask |= NSControlKeyMask; 
			if (desc->macMod & EMGUI_KEY_ALT)
				mask |= NSAlternateKeyMask; 

			NSString* key = convertKeyCodeToString(desc->key);

			if (key)
			{
				[newItem setKeyEquivalentModifierMask: mask];
				[newItem setKeyEquivalent:key];
			}
			else
			{
				fprintf(stderr, "Unable to map keyboard shortcut for %s\n", desc->name);
			}

			[newItem setOnStateImage: newItem.offStateImage];
			[menu addItem:newItem];
			[newItem release];
		}

		desc++;
	}
}	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_buildMenu()
{
 	NSMenu* fileMenu = [[[NSApp mainMenu] itemWithTitle:@"File"] submenu];
 	NSMenu* editMenu = [[[NSApp mainMenu] itemWithTitle:@"Edit"] submenu];
 	NSMenu* viewMenu = [[[NSApp mainMenu] itemWithTitle:@"View"] submenu];

	buildSubMenu(fileMenu, g_fileMenu);
	buildSubMenu(editMenu, g_editMenu);
	buildSubMenu(viewMenu, g_viewMenu);

	Window_setTitle("RocketEditor" EDITOR_VERSION);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_populateRecentList(const char** files)
{
 	NSMenu* fileMenu = [[[NSApp mainMenu] itemWithTitle:@"File"] submenu];
	NSMenu* recentItems = [[fileMenu itemWithTitle:@"Recent Files"] submenu];

	[recentItems removeAllItems];

	for (int i = 0; i < 4; ++i)
	{
		const char* filename = files[i];

		if (!strcmp(filename, ""))
			continue;

		NSString* name = [NSString stringWithUTF8String: filename];

		NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:name action:@selector(onMenuPress:) keyEquivalent:@""];
		[newItem setTag:EDITOR_MENU_RECENT_FILE_0 + i];
		[newItem setRepresentedObject:[NSString stringWithFormat:@"%d",i]];
		[newItem setKeyEquivalentModifierMask: NSCommandKeyMask];
		[newItem setKeyEquivalent:[NSString stringWithFormat:@"%d",i + 1]];

		[recentItems addItem:newItem];

		[newItem release];
	}
}

@end

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void swapBuffers()
{
	[g_context flushBuffer];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window_setTitle(const char* title)
{
	[g_window setTitle:[NSString stringWithUTF8String:title]];
}
