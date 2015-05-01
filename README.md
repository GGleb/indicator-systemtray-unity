System tray indicator for Unity
=================================

![](https://raw.github.com/GGleb/indicator-systemtray-unity/master/indicator-systemtray-unity.png)

Usage
-----

```
sudo apt-get install build-essential libunity-misc-dev libgtk-3-dev libindicator3-dev git-core
git clone https://github.com/GGleb/indicator-systemtray-unity.git
cd indicator-systemtray-unity
make
sudo make install
```
re-login


Deb
-----

```
sudo apt-get install fakeroot dpkg-dev
```
delete line (	glib-compile-schemas $(DESTDIR)/usr/share/glib-2.0/schemas/ ) in Makefile
```
dpkg-buildpackage -rfakeroot -b
```


PPA
-----

```
sudo apt-add-repository ppa:fixnix/indicator-systemtray-unity
sudo apt-get update
sudo apt-get install indicator-systemtray-unity

```
To remove this package (with its configuration files!!!):
```
sudo apt-get --purge remove indicator-systemtray-unity
```

Settings
-----

To change the mode position: press the middle mouse button on the indicator.
The horizontal position can be changed: scrolling the mouse over the indicator.

Settings can be changed in gsettings:/net/launchpad/indicator/systemtray (use dconf-editor).


Credits
-------

Contributors:

- Gleb Golovachev <golovachev.gleb@gmail.com>
