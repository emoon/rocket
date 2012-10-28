#import "RocketView.h"
#include "../Editor.h"
#include <Emgui.h> 
#include <GFXBackend.h> 

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
    
	oglContext = [[NSOpenGLContext alloc] initWithFormat: [NSOpenGLView defaultPixelFormat] shareContext: nil];
	[oglContext makeCurrentContext];

	EMGFXBackend_create();
	Editor_create();

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
		specialKeys = 1;

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

	Emgui_setMousePos((int)location.x, (int)originalFrame.size.height - (int)location.y);
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

	Emgui_setMousePos((int)location.x, (int)originalFrame.size.height - (int)location.y);
	Emgui_setMouseLmb(1);
	
	Editor_update();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(BOOL) isOpaque 
{
    return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(void) dealloc 
{
	Editor_destroy();
	EMGFXBackend_destroy();
    [super dealloc];
}

@end

