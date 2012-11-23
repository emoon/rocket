StaticLibrary {
	Name = "mxml",

	Env = {
		CPPPATH = { ".", "ogl_rocket/external/mxml" },
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
			Dir = "ogl_editor/external/mxml",
			Extensions = { ".c" },
		},
	},
}

StaticLibrary {
	Name = "glfw",

	Env = {
		CPPPATH = { ".", 
			 "ogl_editor/external/glfw/include", 
			 "ogl_editor/external/glfw/lib", 
			 { "ogl_editor/external/glfw/lib/cocoa" ; Config = "macosx-*-*" },
			 { "ogl_editor/external/glfw/lib/win32" ; Config = "win32-*-*" } 
		 },
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
		FGlob {
			Dir = "ogl_editor/external/glfw/lib",
			Extensions = { ".c" },
			Filters = {
				{ Pattern = "x11"; Config = "linux-*-*" },
				{ Pattern = "cocoa"; Config = "macosx-*-*" },
				{ Pattern = "win32"; Config = { "win32-*-*", "win64-*-*" } },
			},
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
						 "ogl_editor/external/glfw/include", 
						 "../emgui/src", 
						 "../../../../../emgui/src",
					     "ogl_editor/External/mxml" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CPPDEFS = {
			{ "EMGUI_MACOSX", Config = "macosx-*-*" },
			{ "ROCKETGUI_WIN32"; Config = { "win32-*-*", "win64-*-*" } },
		},

		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall"; Config = "macosx-clang-*" },
		},
	},

	Sources = { 
		FGlob {
			Dir = "ogl_editor/src",
			Extensions = { ".c", { ".m"; Config = "macosx-*-*" } },
			Filters = {
				{ Pattern = "macosx"; Config = "macosx-*-*" },
				{ Pattern = "windows"; Config = { "win32-*-*", "win64-*-*" } },
			},
		},
	},

	Depends = { "sync", "mxml", "emgui", "glfw" },

	Libs = { { "wsock32.lib", "opengl32.lib", "glu32.lib", "kernel32.lib", "user32.lib", "gdi32.lib" ; Config = "win32-*-*" } },

	Frameworks = { "Cocoa", "OpenGL"  },

}

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

--local native = require('tundra.native')

--if native.host_platform == "macosx" then
--	Default(rocketBundle)
--#else
	Default "editor"
--end



