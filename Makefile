CC=gcc --shared -fPIC
CFLAGS=-g -Wall -Wfatal-errors -std=c99 $(shell pkg-config --cflags --libs gtk+-3.0 indicator3-0.4 unity-misc)

all: libsystemtray.so

libsystemtray.so: indicator-systemtray.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f *.o libsystemtray.so

install:
	install -Dm 644 libsystemtray.so $(DESTDIR)/usr/lib/indicators3/7/libsystemtray.so
	install -Dm 644 resources/indicator-systemtray-unity.png $(DESTDIR)/usr/share/pixmaps/indicator-systemtray-unity.png
	install -Dm 644 resources/UNITY_PANEL_TRAY_DISABLE.sh $(DESTDIR)/etc/profile.d/UNITY_PANEL_TRAY_DISABLE.sh
	install -Dm 644 resources/net.launchpad.indicator.systemtray.gschema.xml $(DESTDIR)/usr/share/glib-2.0/schemas/net.launchpad.indicator.systemtray.gschema.xml
	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/

uninstall:
	rm $(DESTDIR)/usr/lib/indicators3/7/libsystemtray.so
	rm $(DESTDIR)/usr/share/pixmaps/indicator-systemtray-unity.png
	rm $(DESTDIR)/etc/profile.d/UNITY_PANEL_TRAY_DISABLE.sh
	rm $(DESTDIR)/usr/share/glib-2.0/schemas/net.launchpad.indicator.systemtray.gschema.xml
	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/
