libdir.x86_64 := $(shell if [ -d "/usr/lib/x86_64-linux-gnu" ]; then echo "/usr/lib/x86_64-linux-gnu"; else echo "/usr/lib64"; fi )
libdir.i686   := $(shell if [ -d "/usr/lib/i386-linux-gnu" ]; then echo "/usr/lib/i386-linux-gnu"; else echo "/usr/lib"; fi )
libdir.arm    := $(shell if [ -d "/usr/lib/arm-linux-gnueabihf" ]; then echo "/usr/lib/arm-linux-gnueabihf"; else echo "/usr/lib"; fi )
libdir.macos  := /usr/local/lib

ISNOTMACOS := $(shell uname -a | grep "Darwin" >/dev/null ; echo $$? )

ifeq ($(ISNOTMACOS), 0)
	MACHINE := macos
	CFLAGS := -bundle
else
	MACHINE := $(shell uname -m)
	ifneq (, $(findstring armv, $(MACHINE)))
		 MACHINE := arm
	endif
	
	CFLAGS := -shared
	
	ifdef MINGW_PACKAGE_PREFIX
		CFLAGS := $(CFLAGS) -lucrtbase
	endif
endif

libdir = $(libdir.$(MACHINE))
libdir.geany = $(libdir.$(MACHINE))/geany

all: build

build:
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_base64_converter.c -std=c99 -fPIC `pkg-config --cflags geany`
	gcc geany_base64_converter.o -o base64converter.so "$(libdir)/libb64.a" $(CFLAGS) `pkg-config --libs geany`

install: globaluninstall globalinstall localuninstall

uninstall: globaluninstall

globaluninstall:
	rm -f "$(libdir.geany)/base64converter.so"

localuninstall:
	rm -f "$(HOME)/.config/geany/plugins/base64converter.so"

globalinstall:
	cp -f ./base64converter.so "$(libdir.geany)/base64converter.so"
	chmod 755 "$(libdir.geany)/base64converter.so"

localinstall: localuninstall
	mkdir -p "$(HOME)/.config/geany/plugins"
	cp -f ./base64converter.so "$(HOME)/.config/geany/plugins/base64converter.so"
	chmod 755 "$(HOME)/.config/geany/plugins/base64converter.so"

clean:
	rm -f ./base64converter.so
	rm -f ./geany_base64_converter.o
