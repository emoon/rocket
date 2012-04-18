//

#import "RocketView.h"
#include "../GFXBackend.h" 
#include "../RocketGui.h"

extern void Editor_init();
extern void Editor_guiUpdate();
extern RocketGuiState g_rocketGuiState;

@implementation RocketView

- (id)initWithFrame:(NSRect)frame 
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;
    
	// create and activate the context object which maintains the OpenGL state
	oglContext = [[NSOpenGLContext alloc] initWithFormat: [NSOpenGLView defaultPixelFormat] shareContext: nil];
	[oglContext makeCurrentContext];

	GFXBackend_create();
	Editor_init();

	return self;
}

- (void)lockFocus
{
    NSOpenGLContext* context = oglContext;
    
    // make sure we are ready to draw
    [super lockFocus];
    
    // when we are about to draw, make sure we are linked to the view
    // It is not possible to call setView: earlier (will yield 'invalid drawable')
    if ([context view] != self) 
    {
        [context setView:self];
    }
    
    // make us the current OpenGL context
    [context makeCurrentContext];
}

// this is called whenever the view changes (is unhidden or resized)
- (void)drawRect:(NSRect)frameRect 
{
    // inform the context that the view has been resized
    [oglContext update];

    GFXBackend_updateViewPort(frameRect.size.width, frameRect.size.height);
    Editor_guiUpdate();
}

- (void)keyDown:(NSEvent *)theEvent 
{
	if ([theEvent modifierFlags] & NSNumericPadKeyMask) { // arrow keys have this mask
        NSString *theArrow = [theEvent charactersIgnoringModifiers];
        unichar keyChar = 0;
        if ( [theArrow length] == 0 )
            return;            // reject dead keys
        if ( [theArrow length] == 1 ) {
            keyChar = [theArrow characterAtIndex:0];
            if ( keyChar == NSLeftArrowFunctionKey ) {
            	printf("LeftArrow\n");
                return;
            }
            if ( keyChar == NSRightArrowFunctionKey ) {
            	printf("RightArrow\n");
                return;
            }
            if ( keyChar == NSUpArrowFunctionKey ) {
            	printf("UpArrow\n");
                return;
            }
            if ( keyChar == NSDownArrowFunctionKey ) {
            	printf("DownArrow\n");
                return;
            }
            [super keyDown:theEvent];
        }
    }
    [super keyDown:theEvent];
}

- (BOOL)acceptsFirstResponder 
{
    return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

-(void) viewWillMoveToWindow:(NSWindow *)newWindow 
{
    // Setup a new tracking area when the view is added to the window.
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[self frame] 
    	options: (NSTrackingMouseMoved | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseMoved:(NSEvent *)event
{
	NSWindow* window = [self window];
	//NSPoint originalMouseLocation = [window convertBaseToScreen:[event locationInWindow]];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];

	g_rocketGuiState.mousex = (int)location.x; 
	g_rocketGuiState.mousey = (int)originalFrame.size.height - (int)location.y - 23; 

	printf("mouseMoved %d %d\n", g_rocketGuiState.mousex, g_rocketGuiState.mousey);

	Editor_guiUpdate();
}

static NSPoint s_prevDragPos;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseDragged:(NSEvent *)event
{
	NSWindow* window = [self window];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];
	g_rocketGuiState.mousex = (int)location.x; 
	g_rocketGuiState.mousey = (int)originalFrame.size.height - (int)location.y; 
	
	if (g_rocketGuiState.activeItem != -1)
	{
		Editor_guiUpdate();
		return;
	}

	NSPoint newMouseLocation = [window convertBaseToScreen:[event locationInWindow]];
	NSPoint delta = NSMakePoint(newMouseLocation.x - s_prevDragPos.x,
								newMouseLocation.y - s_prevDragPos.y);

	NSRect newFrame = originalFrame;
	
	newFrame.origin.x += delta.x;
	newFrame.origin.y += delta.y;

	s_prevDragPos = newMouseLocation;

	printf("mouseDragged\n");
	
	[window setFrame:newFrame display:YES animate:NO];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseUp:(NSEvent *)event
{
	g_rocketGuiState.mouseDown = 0;
	printf("mouseUp\n");
	Editor_guiUpdate();
}

//
// mouseDown:
//
// Handles mouse clicks in our frame. Two actions:
//	- click in the resize box should resize the window
//	- click anywhere else will drag the window.
//

- (void)mouseDown:(NSEvent *)event
{
	NSWindow *window = [self window];
	s_prevDragPos = [window convertBaseToScreen:[event locationInWindow]];
	NSRect originalFrame = [window frame];
	NSPoint location = [window mouseLocationOutsideOfEventStream];

	g_rocketGuiState.mousex = (int)location.x; 
	g_rocketGuiState.mousey = (int)originalFrame.size.height - (int)location.y - 23; 
	
	g_rocketGuiState.mouseDown = 1;
	printf("mouseDown\n");

	Editor_guiUpdate();
}


-(BOOL) isOpaque 
{
    return YES;
}

-(void) dealloc 
{
	GFXBackend_destroy();
    [super dealloc];
}

@end
