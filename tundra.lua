local macosx = {
	Env = {
		CPPDEFS = { "EMGUI_MACOSX" },
		CCOPTS = {
			"-Wall",
			"-Wno-deprecated-declarations", -- TickCount issue no Mountain Lion (needs to be fixed)
			"-I.", "-DMACOSX", "-Wall",
			{ "-O0", "-g"; Config = "*-*-debug" },
			{ "-O4"; Config = "*-*-release" },
		},
	},

	Frameworks = { "Cocoa" },
}

local win32 = {
	Env = {
 		GENERATE_PDB = "1",
		CCOPTS = {
			"/W4", "/I.", "/WX", "/DUNICODE", "/D_UNICODE", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS", "/wd4996",
			{ "/Od"; Config = "*-*-debug" },
			{ "/O2"; Config = "*-*-release" },
		},
	},
}

Build {
	Units = "units.lua",

	SyntaxExtensions = { "tundra.syntax.glob", "tundra.syntax.osx-bundle" },

	Configs = {
		Config { Name = "macosx-clang", DefaultOnHost = "macosx", Inherit = macosx, Tools = { "clang-osx" } },
		Config { Name = "win32-msvc", DefaultOnHost = { "windows" }, Inherit = win32, Tools = { "msvc" } },
	},
}
