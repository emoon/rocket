require "tundra.syntax.glob"
require "tundra.syntax.osx-bundle"

local GLFW_DIR = "external/glfw/"

StaticLibrary {
    Name = "glfw",

    Env = {
        CPPPATH = {
            GLFW_DIR .. "src",
            GLFW_DIR .. "include",
        },

        CPPDEFS = {
            { "_GLFW_WIN32", "_GLFW_WGL", "WIN32"; Config = "win64-*-*" },
            { "_GLFW_X11", "_GLFW_GFX", "LINUX"; Config = "linux-*-*" },
            { "_GLFW_COCOA", "MACOSX"; Config = "macosx-*-*" },
        },
    },

    Sources = {
        GLFW_DIR .. "src/window.c",
        GLFW_DIR .. "src/context.c",
        GLFW_DIR .. "src/init.c",
        GLFW_DIR .. "src/input.c",
        GLFW_DIR .. "src/monitor.c",
        GLFW_DIR .. "src/vulkan.c",
        GLFW_DIR .. "src/osmesa_context.c",
        GLFW_DIR .. "src/egl_context.c",

        {
            GLFW_DIR .. "src/cocoa_init.m",
            GLFW_DIR .. "src/cocoa_joystick.m",
            GLFW_DIR .. "src/cocoa_monitor.m",
            GLFW_DIR .. "src/cocoa_time.c",
            GLFW_DIR .. "src/cocoa_window.m",
            GLFW_DIR .. "src/posix_thread.c",
            GLFW_DIR .. "src/nsgl_context.h",
            GLFW_DIR .. "src/nsgl_context.m" ; Config = "macosx-*-*"
        },

        {
            GLFW_DIR .. "src/glx_context.c",
            -- GLFW_DIR .. "src/wl_init.c",
            --GLFW_DIR .. "src/wl_monitor.c",
            --GLFW_DIR .. "src/wl_window.c",
            GLFW_DIR .. "src/x11_init.c",
            GLFW_DIR .. "src/x11_monitor.c",
            GLFW_DIR .. "src/x11_window.c",
            GLFW_DIR .. "src/linux_joystick.c",
            GLFW_DIR .. "src/posix_thread.c",
            GLFW_DIR .. "src/posix_time.c",
            GLFW_DIR .. "src/xkb_unicode.c" ; Config = "linux-*-*",
        },

        {
            GLFW_DIR .. "src/wgl_context.c",
            GLFW_DIR .. "src/win32_init.c",
            GLFW_DIR .. "src/win32_joystick.c",
            GLFW_DIR .. "src/win32_monitor.c",
            GLFW_DIR .. "src/win32_thread.c",
            GLFW_DIR .. "src/win32_time.c",
            GLFW_DIR .. "src/win32_window.c" ; Config = "win64-*-*",
        },
    },
}

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
	Name = "tinycthread",

	Env = {
		CPPPATH = { ".", "external/tinycthread" },
		PROGOPTS = {
			{ "/SUBSYSTEM:WINDOWS", "/DEBUG"; Config = { "win32-*-*", "win64-*-*" } },
		},
	},

	Sources = {
		Glob {
			Dir = "external/tinycthread",
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

	Env = {
		CPPPATH = { ".", "external/rocket/lib" },
	},

	Sources = {
		Glob {
			Dir = "external/rocket/lib",
			Extensions = { ".c" },
		},
	},
}


Program {
	Name = "editor",

	Env = {
	    LIBPATH = {
	        { "external/bass/win64"; Config = "win32-*-*" },
	        { "external/bass/mac"; Config = "macosx-*-*" },
	        { "external/bass/linux"; Config = "linux-*-*" },
	    },

		CPPPATH = { ".", "src",
						 "emgui/include",
						 "external/rocket/lib",
						 "external/tinycthread",
						 "external/bass",
					     "external/mxml" },
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

	Depends = { "sync", "mxml", "emgui", "tinycthread", "glfw" },

	Libs = {
		{ "wsock32.lib", "opengl32.lib", "glu32.lib", "kernel32.lib",
		   "user32.lib", "gdi32.lib", "Comdlg32.lib", "Advapi32.lib", "bass.lib" ; Config = "win32-*-*" },
		{ "GL", "SDL2", "m", "bass"; Config = "linux-*-*" },
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
	MacOSFiles = { "external/bass/mac/libbass.dylib"},
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


