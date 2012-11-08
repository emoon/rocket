local macosx = {
	Env = {
		CPPDEFS = { "EMGUI_MACOSX" },
		CCOPTS = {
			-- "-Weverything",
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
			"/W4", "/I.", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS",
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
