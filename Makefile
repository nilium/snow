DEBUG=
TARGET=unknown
OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	TARGET=osx
endif

APP_OUT=bin/snow

CFLAGS+= -Wall
CFLAGS+= -DTARGET_OS_MAC
CFLAGS+= -fblocks

PKGCONFIG_LIBS=sqlite3 glfw3 libenet snow-common
CFLAGS+= $(shell pkg-config --cflags $(PKGCONFIG_LIBS))
LDFLAGS+= $(shell pkg-config --libs $(PKGCONFIG_LIBS))
LDFLAGS+= -llua
LDFLAGS+= -lphysfs

ifeq ($(DEBUG),)
	DEBUG=yes
endif

ifeq ($(DEBUG),yes)
	CFLAGS+= -g -O0
	LDFLAGS+= -g
else
	CFLAGS+= -O3 -DNDEBUG
endif

include Makefile.$(TARGET)

.PHONY: all clean deps Makefile.sources

all: $(APP_OUT)

deps:
	./build-sources.rb --target $(TARGET) > Makefile.sources

clean:
	$(RM) $(APP_OUT) $(OBJECTS)

include Makefile.sources

$(APP_OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
