local macosx = {
	Env = {
		CPPDEFS = { "EMGUI_MACOSX" },
		CCOPTS = {
			"-Wall",
			"-Wno-format-security",
			"-Wno-deprecated-declarations", -- TickCount issue no Mountain Lion (needs to be fixed)
			"-I.", "-DMACOSX", "-Wall",
			{ "-O0", "-g"; Config = "*-*-debug" },
			{ "-O3"; Config = "*-*-release" },
		},
	},

	Frameworks = { "Cocoa" },
}

local win32 = {
	Env = {
 		GENERATE_PDB = "1",
		CCOPTS = {
			"/W4", "/I.", "/WX", "/DUNICODE", "/FS", "/D_UNICODE", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS", "/wd4996", "/wd4389",
			{ "/Od"; Config = "*-*-debug" },
			{ "/O2"; Config = "*-*-release" },
		},
	},
}

local linux = {
	Env = {
		CPPDEFS = { "EMGUI_UNIX" },
		CCOPTS = {
			"-I.",
			"`sdl-config --cflags`",
			{ "-O0", "-g"; Config = "*-*-debug" },
			{ "-O2"; Config = "*-*-release" },
		},
	},
}

Build {
	Units = "units.lua",

	Configs = {
		Config { Name = "macosx-clang", DefaultOnHost = "macosx", Inherit = macosx, Tools = { "clang-osx" } },
		Config { Name = "win32-msvc", DefaultOnHost = { "windows" }, Inherit = win32, Tools = { "msvc" } },
		Config { Name = "linux-gcc", DefaultOnHost = { "linux" }, Inherit = linux, Tools = { "gcc" } },
	},
}
