CC=gcc --shared -fPIC -Wall
CFLAGS=-Wfatal-errors $(shell pkg-config --cflags --libs gtk+-3.0)
CFLAGS_IND=-Wfatal-errors $(shell pkg-config --cflags --libs gtk+-3.0 indicator3-0.4)
CFLAGS_MAIN=-Wfatal-errors -std=c99 $(shell pkg-config --cflags --libs gtk+-3.0 x11) -lm

all : libsystemtray.so clean

libsystemtray.so : indicator-systemtray.o fixedtip.o gnome-bg-slideshow.o na-marshal.o na-tray.o na-tray-child.o na-tray-manager.o
	$(CC) -o $@ $^ $(CFLAGS_MAIN)

indicator-systemtray.o : indicator-systemtray.c
	$(CC) -c $< $(CFLAGS_IND)

fixedtip.o : fixedtip.c
	$(CC) -c $< $(CFLAGS)

gnome-bg-slideshow.o : gnome-bg-slideshow.c
	$(CC) -c $< $(CFLAGS)

na-marshal.o : na-marshal.c
	$(CC) -c $< $(CFLAGS)

na-tray.o : na-tray.c
	$(CC) -c $< $(CFLAGS)

na-tray-child.o : na-tray-child.c
	$(CC) -c $< $(CFLAGS)

na-tray-manager.o : na-tray-manager.c
	$(CC) -c $< $(CFLAGS)

clean :
	rm -f *.o fixedtip gnome-bg-slideshow na-marshal na-tray na-tray-child na-tray-manager

install:
	install -Dm 644 libsystemtray.so $(DESTDIR)/usr/lib/indicators3/7/libsystemtray.so
	install -Dm 644 resources/indicator-systemtray-unity.png $(DESTDIR)/usr/share/pixmaps/indicator-systemtray-unity.png
	install -Dm 644 resources/UNITY_PANEL_TRAY_DISABLE.sh $(DESTDIR)/etc/profile.d/UNITY_PANEL_TRAY_DISABLE.sh
	install -Dm 644 resources/net.launchpad.indicator.systemtray.gschema.xml $(DESTDIR)/usr/share/glib-2.0/schemas/net.launchpad.indicator.systemtray.gschema.xml
	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/
	bash potomo.sh $(DESTDIR)/usr/share/locale

uninstall:
	rm $(DESTDIR)/usr/lib/indicators3/7/libsystemtray.so
	rm $(DESTDIR)/usr/share/pixmaps/indicator-systemtray-unity.png
	rm $(DESTDIR)/etc/profile.d/UNITY_PANEL_TRAY_DISABLE.sh
	rm $(DESTDIR)/usr/share/glib-2.0/schemas/net.launchpad.indicator.systemtray.gschema.xml
	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/
