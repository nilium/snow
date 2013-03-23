DEBUG=
TARGET=unknown
OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	TARGET=osx
endif

include Makefile.$(TARGET)

APP_OUT=bin/snow

CFLAGS+= -Wall
CFLAGS+= -DTARGET_OS_MAC
CFLAGS+= -fblocks

PKGCONFIG_LIBS=glfw3 libenet snow-common
CFLAGS+= $(shell pkg-config --cflags $(PKGCONFIG_LIBS))
LDFLAGS+= $(shell pkg-config --libs $(PKGCONFIG_LIBS))

ifeq ($(DEBUG),)
	DEBUG=yes
endif

ifeq ($(DEBUG),yes)
	CFLAGS+= -g -O0
	LDFLAGS+= -g
else
	CFLAGS+= -O3 -DNDEBUG
endif

.PHONY: all clean deps Makefile.sources

all: $(APP_OUT)

deps:
	./build-sources.rb --target $(TARGET) > Makefile.sources

include Makefile.$(TARGET)
include Makefile.sources

clean:
	$(RM) $(APP_OUT) $(OBJECTS)

$(APP_OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
