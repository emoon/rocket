StaticLibrary {
	Name = "mxml",

	Env = {
		CPPPATH = { "." },
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
		CPPPATH = { ".", "ogl_editor/src" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "ROCKETGUI_MACOSX", Config = "macosx-*-*" },
			{ "ROCKETGUI_WIN32"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall"; Config = "macosx-clang-*" },
		},
	},

	Depends = { "sync", "mxml" },

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


