CC=gcc -std=c99
FLAGS := $(shell pkg-config --cflags gtk+-2.0)
LIBS := $(shell pkg-config --libs gtk+-2.0)
INSTALL = /usr/bin/install
STRIP = /usr/bin/strip
prefix = /usr
bindir = ${prefix}/bin
sbindir = ${prefix}/sbin
datarootdir = ${prefix}/share
libdir = /lib
sysconfdir = /etc

all:
	$(CC) -o bluez-tray src/bluez-tray.c -lbluetooth $(FLAGS) $(LIBS)
	$(CC) -o bt-scan src/bt-scan.c -lbluetooth
	
install:
	$(INSTALL) -D -m 755 bluez-tray $(DESTDIR)$(bindir)/bluez-tray
	$(STRIP) $(DESTDIR)$(bindir)/bluez-tray
	$(INSTALL) -D -m 755 bt-scan $(DESTDIR)$(bindir)/bt-scan
	$(STRIP) $(DESTDIR)$(bindir)/bt-scan
	
	$(INSTALL) -D -m 644 bluez-tray.mo $(DESTDIR)$(datarootdir)/locale/ru/LC_MESSAGES/bluez-tray.mo
	cp -a pixmaps/ $(DESTDIR)$(datarootdir)/
	$(INSTALL) -D -m 755 bluez.sh $(DESTDIR)$(libdir)/udev/bluez.sh
	$(INSTALL) -D -m 644 97-bluetooth.rules $(DESTDIR)$(libdir)/udev/rules.d/97-bluetooth.rules

clean:
	rm bluez-tray bt-scan
