#include "GraphView.h"
#include <emgui/Emgui.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GraphView_render(const GraphView* graphView, const GraphSettings* settings, const Rect* rect)
{
	//<F7>float* 



	Emgui_drawBorder(settings->borderColor, settings->borderColor, 
					 rect->x, rect->y, rect->width, rect->height);
}



