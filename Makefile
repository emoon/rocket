# default target
all:

# default build flags
CFLAGS = -g -Wall
LDLIBS = -lm

# user-defined config file (if available)
-include config.mak

# windows wants .exe extension for executables
ifdef COMSPEC
	X = .exe
endif

OBJS = \
	sync/data.o \
	sync/device.o \
	sync/track.o

all: lib/librocket.a

bin/example_bass$X: lib/librocket.a
bin/example_bass$X: LDLIBS += examples/bass.lib d3dx9.lib -ld3d9 -lws2_32

bin/example_sdl$X: lib/librocket.a
bin/example_sdl$X: LDLIBS += -lSDL -lSDL_mixer -lGL -lGLU

clean:
	$(RM) -rf $(OBJS) lib bin/example_bass$X bin/example_sdl$X

lib/librocket.a: $(OBJS)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^

bin/%$X: examples/%.c
	@mkdir -p bin
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

bin/%$X: examples/%.cpp
	@mkdir -p bin
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@
