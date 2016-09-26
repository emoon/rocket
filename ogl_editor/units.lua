require "tundra.syntax.glob"
require "tundra.syntax.osx-bundle"

StaticLibrary {
	Name = "mxml",

	Env = {
		CPPPATH = { ".", "external/mxml" },
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
			Dir = "external/mxml",
			Extensions = { ".c" },
		},
	},
}

StaticLibrary {
	Name = "emgui",

	Env = {
		CPPPATH = { ".", "emgui/src", "emgui/include" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "EMGUI_MACOSX", Config = "macosx-*-*" },
			{ "EMGUI_WINDOWS"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall", "-Wno-format-security"; Config = "macosx-clang-*" },
		},
	},

	Sources = {
		FGlob {
			Dir = "emgui/src",
			Extensions = { ".c", ".h" },
			Filters = {
				{ Pattern = "macosx"; Config = "linux-*-*" },
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
			Dir = "../lib",
			Extensions = { ".c" },
		},
	},
}


Program {
	Name = "editor",

	Env = {
	    LIBPATH = {
	        { "External/bass/win32"; Config = "win32-*-*" },
	        { "External/bass/mac"; Config = "macosx-*-*" },
	        { "External/bass/linux"; Config = "linux-*-*" },
	    },

		CPPPATH = { ".", "src",
						 "emgui/include",
						 "External/bass",
					     "External/mxml" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "EMGUI_MACOSX", Config = "macosx-*-*" },
			{ "ROCKETGUI_WIN32"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall", "-Wno-format-security"; Config = "macosx-clang-*" },
		},
	},

	Sources = {
		FGlob {
			Dir = "src",
			Extensions = { ".c", ".m", ".h" },
			Filters = {
				{ Pattern = "linux"; Config = "linux-*-*" },
				{ Pattern = "macosx"; Config = "macosx-*-*" },
				{ Pattern = "windows"; Config = { "win32-*-*", "win64-*-*" } },
			},
		},

		{ "data/windows/editor.rc" ; Config = { "win32-*-*", "win64-*-*" } },
	},

	Depends = { "sync", "mxml", "emgui" },

	Libs = {
		{ "wsock32.lib", "opengl32.lib", "glu32.lib", "kernel32.lib",
		   "user32.lib", "gdi32.lib", "Comdlg32.lib", "Advapi32.lib", "bass.lib" ; Config = "win32-*-*" },
		{ "GL", "SDL", "m", "bass"; Config = "linux-*-*" },
		{ "bass"; Config = "macosx-*-*" },
	},

	Frameworks = { "Cocoa", "OpenGL", "Carbon", "CoreAudio"  },
}

Program {
	Name = "basic_example",
	Sources = { "basic_example/basic_example.c" },
	Depends = { "sync" },
	Libs = {
		{ "wsock32.lib", "ws2_32.lib"; Config = "win32-*-*" },
	},
}

local rocketBundle = OsxBundle
{
	Depends = { "editor" },
	Target = "$(OBJECTDIR)/RocketEditor.app",
	InfoPList = "data/macosx/Info.plist",
	Executable = "$(OBJECTDIR)/editor",
	Resources = {
		CompileNib { Source = "data/macosx/appnib.xib", Target = "appnib.nib" },
		"data/macosx/icon.icns",
	},
}

local native = require('tundra.native')

if native.host_platform == "macosx" then
	Default(rocketBundle)
else
	Default "editor"
end

Default "basic_example"


