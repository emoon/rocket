//

#import "RocketView.h"
#include <OpenGLRenderer/OpenGLRenderer.h> 

extern void Editor_init();
extern void Editor_guiUpdate();

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

- (void)mouseDown:(NSEvent *)theEvent 
{
	printf("mouseDown\n");
    [[self nextResponder] mouseDown:theEvent];
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
