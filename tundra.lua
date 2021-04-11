local macosx = {
	Env = {
		CPPDEFS = { "EMGUI_MACOSX" },
		CCOPTS = {
			"-Wall",
			"-mmacosx-version-min=10.7",
			"-Wno-format-security",
			"-Wno-deprecated-declarations", -- TickCount issue no Mountain Lion (needs to be fixed)
			"-I.", "-DMACOSX", "-Wall",
			{ "-O0", "-g"; Config = "*-*-debug" },
			{ "-O3"; Config = "*-*-release" },
		},

		PROGOPTS = {
			"-mmacosx-version-min=10.7",
		}
	},

	Frameworks = { "Cocoa" },
}

local win32 = {
	Env = {
		CPPDEFS = { "EMGUI_WINDOWS" },
 		GENERATE_PDB = "1",
		CCOPTS = {
			"/W4", "/I.", "/WX", "/DUNICODE", "/FS", "/D_UNICODE", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS", "/wd4996", "/wd4389", "/Wv:18",
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
			"`sdl2-config --cflags`",
			{ "-O0", "-g"; Config = "*-*-debug" },
			{ "-O2"; Config = "*-*-release" },
		},

        LIBS = {
            {"pthread", "m"; Config = "linux-*-*" },
        },
	},
}

Build {
	Units = "units.lua",

	Configs = {
		Config { Name = "macosx-clang", DefaultOnHost = "macosx", Inherit = macosx, Tools = { "clang-osx" } },
		Config { Name = "win32-msvc", DefaultOnHost = { "windows" }, Inherit = win32, Tools = { "msvc-vs2019" } },
		Config { Name = "win32_msvc-bare", Inherit = win32, Tools = { "msvc" } },
		Config { Name = "linux-gcc", DefaultOnHost = { "linux" }, Inherit = linux, Tools = { "gcc" } },
	},

    IdeGenerationHints = {
        Msvc = {
            PlatformMappings = {
                ['win32-msvc'] = 'Win32',
            },

            VariantMappings = {
                ['production'] = 'Production',
                ['release'] = 'Release',
                ['debug'] = 'Debug',
            },
        },

		MsvcSolutions = {
			['RocketEditor.sln'] = { } -- will get everything
		},
    },
}
