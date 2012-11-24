#import "RocketView.h"
#include "../Editor.h"
#include "../rlog.h"
#include <Emgui.h> 
#include <GFXBackend.h> 

NSOpenGLContext* g_context = 0;

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

    NSOpenGLPixelFormatAttribute attributes[4];

    attributes[0] = NSOpenGLPFADoubleBuffer;
    attributes[1] = 0;

    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    oglContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
	//oglContext = [[NSOpenGLContext alloc] initWithFormat: [NSOpenGLView defaultPixelFormat] shareContext: nil];
	[oglContext makeCurrentContext];

	g_context = oglContext;

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

	EMGFXBackend_updateViewPort((int)frameRect.size.width, (int)frameRect.size.height);
	Editor_setWindowSize((int)frameRect.size.width, (int)frameRect.size.height);
    Editor_update();
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
	int specialKeys = 0;

	if ([theEvent modifierFlags] & NSShiftKeyMask)
		specialKeys |= EMGUI_KEY_SHIFT;

	if ([theEvent modifierFlags] & NSAlternateKeyMask)
		specialKeys |= EMGUI_KEY_ALT;

	if ([theEvent modifierFlags] & NSControlKeyMask)
		specialKeys |= EMGUI_KEY_CTRL;

	if ([theEvent modifierFlags] & NSCommandKeyMask)
		specialKeys |= EMGUI_KEY_COMMAND;

	Emgui_sendKeyinput(keyChar, specialKeys);

	if ([theEvent modifierFlags] & NSNumericPadKeyMask) 
	{ 
		switch (keyChar)
		{
			case NSLeftArrowFunctionKey: keyCode = EMGUI_ARROW_LEFT; break;
			case NSRightArrowFunctionKey: keyCode = EMGUI_ARROW_RIGHT; break;
			case NSUpArrowFunctionKey: keyCode = EMGUI_ARROW_UP; break;
			case NSDownArrowFunctionKey: keyCode = EMGUI_ARROW_DOWN; break;
		}
	}

	if (!Editor_keyDown(keyCode, specialKeys))
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
    	options: (NSTrackingMouseMoved | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
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

@end

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void swapBuffers()
{
	[g_context flushBuffer];
}
