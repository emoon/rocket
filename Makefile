# default target
all:

# default build flags
CFLAGS = -g -O2 -Wall
CPPFLAGS = -I.

# user-defined config file (if available)
-include config.mak

# windows wants .exe extension for executables
ifdef COMSPEC
	X = .exe
endif

SYNC_OBJS = \
	sync/data.o \
	sync/device.o \
	sync/track.o

all: lib/librocket.a bin/example_bass$X

bin/example_bass$X: LDLIBS += examples/bass.lib d3dx9.lib lib/librocket.a \
	-ld3d9 -lws2_32

clean:
	$(RM) -rf $(SYNC_OBJS) lib

lib/librocket.a: $(SYNC_OBJS)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^

bin/%$X: examples/%.cpp
	@mkdir -p bin
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@
