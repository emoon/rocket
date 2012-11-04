
Build {
	Units = "units.lua",

	SyntaxExtensions = { "tundra.syntax.glob", "tundra.syntax.osx-bundle" },

	Configs = {
		{
			Name = "macosx-clang",
			DefaultOnHost = "macosx",
			Tools = { "clang-osx" },
			Env = {
				LIBPATH = { "/usr/lib/gcc/i686-apple-darwin10/4.2.1/x86_64" },
				CPPDEFS = { "SDLROCKET_MACOSX" },
				CCOPTS = {
                    { "-g", "-O0" ; Config = { "*-gcc-debug", "*-clang-debug" } },
                    { "-g", "-O3" ; Config = { "*-clang-release" } }					
        		},
				CXXOPTS = {
					{ "-g", "-O0"; Config = "macosx-clang-debug" },
				},
			},
		},
	},
}
