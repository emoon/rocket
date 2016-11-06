#include "Menu.h"
#include <emgui/Emgui.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MenuDescriptor g_fileMenu[] =
{
	{ _T("Open..."), 		EDITOR_MENU_OPEN,			'o', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Recent Files"),	EDITOR_MENU_SUB_MENU,		 0, 	0, 0 },
	{ _T(""), 				EDITOR_MENU_SEPARATOR,		 0, 	0, 0 },
	{ _T("Save"), 			EDITOR_MENU_SAVE,			's', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Save as..."), 	EDITOR_MENU_SAVE_AS,		's', 	EMGUI_KEY_COMMAND | EMGUI_KEY_SHIFT, EMGUI_KEY_CTRL | EMGUI_KEY_SHIFT },
	{ _T("Remote export"),	EDITOR_MENU_REMOTE_EXPORT,	'e', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T(""), 		        EDITOR_MENU_SEPARATOR,		 0,     0, 0 },
	{ _T("Load Music.."),	EDITOR_MENU_LOAD_MUSIC,	    'l', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ 0 },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MenuDescriptor g_editMenu[] =
{
	{ _T("Undo"), 					EDITOR_MENU_UNDO,			'z', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Redo"), 					EDITOR_MENU_REDO,			'z',	EMGUI_KEY_COMMAND | EMGUI_KEY_SHIFT, EMGUI_KEY_CTRL | EMGUI_KEY_SHIFT },
	{ _T("Cancel Edit"), 			EDITOR_MENU_CANCEL_EDIT,	EMGUI_KEY_ESC, 	0, 0 },
	{ _T("Delete Key"),				EDITOR_MENU_DELETE_KEY,		EMGUI_KEY_BACKSPACE, 	0, 0 },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		0, 				0, 0 },
	{ _T("Cut"), 					EDITOR_MENU_CUT,			'x', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Copy"), 					EDITOR_MENU_COPY,			'c', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Paste"), 					EDITOR_MENU_PASTE,			'v',	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Move selection up"),		EDITOR_MENU_MOVE_UP,		'c',	0 , 0 },
	{ _T("Move selection down"),	EDITOR_MENU_MOVE_DOWN,		'x',	0 , 0 },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		  0, 				0, 0 },
	{ _T("Select Track"), 			EDITOR_MENU_SELECT_TRACK,	't',	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		  0, 				0, 0 },
	{ _T("Bias +0.01"), 			EDITOR_MENU_BIAS_P_001,		'q', 	0, 0 },
	{ _T("Bias +0.1"), 				EDITOR_MENU_BIAS_P_01,		'w', 	0, 0 },
	{ _T("Bias +1.0"), 				EDITOR_MENU_BIAS_P_1,		'e', 	0, 0 },
	{ _T("Bias +10.0"), 			EDITOR_MENU_BIAS_P_10,		'r', 	0, 0 },
	{ _T("Bias +10.0"), 			EDITOR_MENU_BIAS_P_100,		't', 	0, 0 },
	{ _T("Bias +100.0"), 			EDITOR_MENU_BIAS_P_1000,	'y', 	0, 0 },
	{ _T("Bias -0.01"), 			EDITOR_MENU_BIAS_N_001,		'a', 	0, 0 },
	{ _T("Bias -0.1"), 				EDITOR_MENU_BIAS_N_01,		's', 	0, 0 },
	{ _T("Bias -1.0"), 				EDITOR_MENU_BIAS_N_1,		'd', 	0, 0 },
	{ _T("Bias -10.0"), 			EDITOR_MENU_BIAS_N_10,		'f', 	0, 0 },
	{ _T("Bias -10.0"),		 		EDITOR_MENU_BIAS_N_100,		'g', 	0, 0 },
	{ _T("Bias -100.0"), 			EDITOR_MENU_BIAS_N_1000,	'h', 	0, 0 },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		  0, 	0, 0 },
	{ _T("Scale 1.01"),				EDITOR_MENU_SCALE_101,		'q',	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 1.1"), 				EDITOR_MENU_SCALE_11,		'w', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 1.2"), 				EDITOR_MENU_SCALE_12,		'e', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 5.0"), 				EDITOR_MENU_SCALE_5,		'r', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 10.0"), 			EDITOR_MENU_SCALE_100,		't', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 100.0"), 			EDITOR_MENU_SCALE_1000,		'y', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.99"), 			EDITOR_MENU_SCALE_099,		'a', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.9"), 				EDITOR_MENU_SCALE_09,		's', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.8"), 				EDITOR_MENU_SCALE_08,		'd', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.5"), 				EDITOR_MENU_SCALE_05,		'f', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.1"),		 		EDITOR_MENU_SCALE_01,		'g', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T("Scale 0.01"), 			EDITOR_MENU_SCALE_001,		'h', 	EMGUI_KEY_SHIFT, EMGUI_KEY_SHIFT },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		  0, 	0, 0 },
	{ _T("Interpolation"),			EDITOR_MENU_INTERPOLATION,	'i', 	0, 0 },
	{ _T("Invert Selection"),		EDITOR_MENU_INVERT_SELECTION,	'i', 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL  },
	{ _T("Insert current value"),	EDITOR_MENU_ENTER_CURRENT_V,EMGUI_KEY_ENTER,0, 0 },
	{ _T(""), 						EDITOR_MENU_SEPARATOR,		  0, 	0, 0 },
	{ _T("Mute/Unmute track"),		EDITOR_MENU_MUTE_TRACK,		'm', 	0, 0 },
	{ 0 },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MenuDescriptor g_viewMenu[] =
{
	{ _T("Start/Stop Playback"), 		EDITOR_MENU_PLAY,				EMGUI_KEY_SPACE, 		0, 0 },
	{ _T("Start Loop Playback"), 		EDITOR_MENU_PLAY_LOOP,			EMGUI_KEY_SPACE, 		EMGUI_KEY_CTRL, EMGUI_KEY_CTRL },
	{ _T(""), 							EDITOR_MENU_SEPARATOR,			0, 						0, 0 },
	{ _T("Jump 8 rows up"), 			EDITOR_MENU_ROWS_UP,			EMGUI_KEY_ARROW_UP, 	EMGUI_KEY_ALT, EMGUI_KEY_ALT },
	{ _T("Jump 8 rows down"), 			EDITOR_MENU_ROWS_DOWN,			EMGUI_KEY_ARROW_DOWN, 	EMGUI_KEY_ALT, EMGUI_KEY_ALT },
	{ _T("Jump 16 rows up"), 			EDITOR_MENU_ROWS_2X_UP,			EMGUI_KEY_PAGE_UP, 		0, 0 },
	{ _T("Jump 16 rows down"), 			EDITOR_MENU_ROWS_2X_DOWN,		EMGUI_KEY_PAGE_DOWN, 	0, 0 },
	{ _T("Jump to previous bookmark"), 	EDITOR_MENU_PREV_BOOKMARK,		EMGUI_KEY_ARROW_UP, 	EMGUI_KEY_COMMAND, EMGUI_KEY_ALT | EMGUI_KEY_CTRL },
	{ _T("Jump to next bookmark"),		EDITOR_MENU_NEXT_BOOKMARK,		EMGUI_KEY_ARROW_DOWN, 	EMGUI_KEY_COMMAND, EMGUI_KEY_ALT | EMGUI_KEY_CTRL },
	{ _T("Jump to previous key"), 		EDITOR_MENU_PREV_KEY,			EMGUI_KEY_ARROW_UP, 	EMGUI_KEY_CTRL, EMGUI_KEY_CTRL },
	{ _T("Jump to next key"),			EDITOR_MENU_NEXT_KEY,			EMGUI_KEY_ARROW_DOWN, 	EMGUI_KEY_CTRL, EMGUI_KEY_CTRL },
	{ _T(""), 							EDITOR_MENU_SEPARATOR,			0, 						0, 0 },
	{ _T("Scroll view Right"), 			EDITOR_MENU_SCROLL_RIGHT,		EMGUI_KEY_ARROW_RIGHT, 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Scroll view Left"),			EDITOR_MENU_SCROLL_LEFT,		EMGUI_KEY_ARROW_LEFT, 	EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T(""), 							EDITOR_MENU_SEPARATOR,			0, 						0, 0 },
	{ _T("Fold track"),					EDITOR_MENU_FOLD_TRACK,			EMGUI_KEY_ARROW_LEFT, 	EMGUI_KEY_ALT, EMGUI_KEY_ALT },
	{ _T("Unfold track"),				EDITOR_MENU_UNFOLD_TRACK,		EMGUI_KEY_ARROW_RIGHT, 	EMGUI_KEY_ALT, EMGUI_KEY_ALT },
	{ _T("Fold group"),					EDITOR_MENU_FOLD_GROUP,			EMGUI_KEY_ARROW_LEFT, 	EMGUI_KEY_ALT | EMGUI_KEY_CTRL, EMGUI_KEY_ALT | EMGUI_KEY_CTRL },
	{ _T("Unfold group"),				EDITOR_MENU_UNFOLD_GROUP,		EMGUI_KEY_ARROW_RIGHT, 	EMGUI_KEY_ALT | EMGUI_KEY_CTRL, EMGUI_KEY_ALT | EMGUI_KEY_CTRL },
	{ _T(""), 							EDITOR_MENU_SEPARATOR,			0, 						0, 0 },
	{ _T("Toogle bookmark"),			EDITOR_MENU_TOGGLE_BOOKMARK,	'b', 					0, 0 },
	{ _T("Clear bookmarks"),			EDITOR_MENU_CLEAR_BOOKMARKS,	'b', 					EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T(""), 							EDITOR_MENU_SEPARATOR,			0, 						0, 0 },
	{ _T("Toogle loopmark"),			EDITOR_MENU_TOGGLE_LOOPMARK,	'z', 					0, 0 },
	{ _T("Clear loopmarks"),			EDITOR_MENU_CLEAR_LOOPMARKS,	'z', 					EMGUI_KEY_COMMAND, EMGUI_KEY_CTRL },
	{ _T("Jump row,start/end & edit"), 	EDITOR_MENU_TAB,				EMGUI_KEY_TAB, 			0, 0 },
	{ 0 },
};
