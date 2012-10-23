StaticLibrary {
	Name = "mxml",

	Env = {
		CPPPATH = { ".", "ogl_rocket/src/External/mxml" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{"_THREAD_SAFE", "_REENTRANT"; Config = "macosx-*-*" }
		},

		CCOPTS = {
			{ "-Wall"; Config = "macosx-clang-*" },
		},
	},

	Sources = { 
		Glob {
			Dir = "ogl_editor/src/External/mxml",
			Extensions = { ".c" },
		},
	},
}

StaticLibrary {
	Name = "emgui",

	Env = {
		CPPPATH = { ".", "../../emgui/src", "../../../../emgui/src" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "EMGUI_MACOSX", Config = "macosx-*-*" },
			{ "EMGUI_WINDOWS"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall"; Config = "macosx-clang-*" },
		},
	},

	Sources = { 
		FGlob {
			Dir = "../emgui/src",
			Extensions = { ".c" },
			Filters = {
				{ Pattern = "macosx"; Config = "macosx-*-*" },
				{ Pattern = "windows"; Config = { "win32-*-*", "win64-*-*" } },
			},
		},
	},
}


StaticLibrary {
	Name = "sync",

	Sources = { 
		Glob {
			Dir = "sync",
			Extensions = { ".c" },
		},
	},
}


Program {
	Name = "editor",

	Env = {
		CPPPATH = { ".", "ogl_editor/src", 
						 "../emgui/src", "../../../../../emgui/src",
					     "ogl_editor/External/mxml" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "EMGUI_MACOSX", Config = "macosx-*-*" },
			{ "ROCKETGUI_WIN32"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall"; Config = "macosx-clang-*" },
		},
	},

	Depends = { "sync", "mxml", "emgui" },

	Frameworks = { "Cocoa", "OpenGL"  },

	Sources = { 
		FGlob {
			Dir = "ogl_editor/src",
			Extensions = { ".c", ".m" },
			Filters = {
				{ Pattern = "macosx"; Config = "macosx-*-*" },
				{ Pattern = "windows"; Config = { "win32-*-*", "win64-*-*" } },
			},
		},
	},
}

Default "editor"

local rocketBundle = OsxBundle 
{
	Depends = { "editor" },
	Target = "$(OBJECTDIR)/RocketEditor.app",
	InfoPList = "ogl_editor/data/macosx/Info.plist",
	Executable = "$(OBJECTDIR)/editor",
	Resources = {
		CompileNib { Source = "ogl_editor/data/macosx/appnib.xib", Target = "appnib.nib" },
		"ogl_editor/data/macosx/icon.icns",
	},
}

Default(rocketBundle)


