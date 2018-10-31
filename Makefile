CC=gcc -std=c99
FLAGS := $(shell pkg-config --cflags gtk+-2.0)
LIBS := $(shell pkg-config --libs gtk+-2.0)
INSTALL = /usr/bin/install
STRIP = /usr/bin/strip
prefix = /usr
bindir = ${prefix}/bin
sbindir = ${prefix}/sbin
datarootdir = ${prefix}/share
libdir = ${prefix}/lib
sysconfdir = /etc

all:
	$(CC) -o bluez-tray src/bluez-tray.c -lbluetooth $(FLAGS) $(LIBS)
	$(CC) -o bt-scan src/bt-scan.c -lbluetooth
	
install:
	$(INSTALL) -D -m 755 bluez-tray $(DESTDIR)$(bindir)/bluez-tray
	$(INSTALL) -D -m 755 bt-scan $(DESTDIR)$(bindir)/bt-scan
	
	$(INSTALL) -d $(DESTDIR)$(datarootdir)/
	cp -a pixmaps/ $(DESTDIR)$(datarootdir)/
	
clean:
	rm bluez-tray bt-scan
