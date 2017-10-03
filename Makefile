CC=gcc
FLAGS := $(shell pkg-config --cflags gtk+-2.0)
LIBS := $(shell pkg-config --libs gtk+-2.0)
SOURCES= bluez-tray.c -lbluetooth
bluez-tray : $(SOURCES)
	$(CC) -o $@ $(SOURCES) $(FLAGS) $(LIBS)
