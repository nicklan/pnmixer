PNMixer [![Build Status](https://travis-ci.org/nicklan/pnmixer.svg?branch=master)](https://travis-ci.org/nicklan/pnmixer)
=======

PNMixer is a simple mixer application designed to run in your system
tray.  It integrates nicely into desktop environments that don't have
a panel that supports applets, and therefore can't run a mixer applet.
In particular it's been used quite a lot with fbpanel and tint2, but
should run fine in any system tray.

PNMixer is designed to work on systems that use ALSA for sound management.
Any other sound driver like OSS or FFADO, or sound server like PulseAudio
or Jackd, are currently not supported (patches welcome).

PNMixer is a fork of [OBMixer](http://jpegserv.com/?page_id=282) with
a number of additions.  These include:

- Volume adjustment with the scroll wheel
- Select which ALSA device and channel to use
- Detect disconnect from sound system and re-connect if requested
- Bind and use HotKeys for volume control
- Texual display of volume level in popup window
- Continous volume adjustment when dragging the slider (not just when you let go)
- Draw a volume level onto system tray icon
- Use system icon theme for icons and use mute/low/medium/high
  volume icons
- Configurable middle click action
- Preferences for:
	- volume text display
	- volume text position
	- icon theme
	- amount to adjust per scoll
	- middle click action
	- drawing of volume level on tray icon

Source and so on are at:
[https://github.com/nicklan/pnmixer](https://github.com/nicklan/pnmixer)

Icons
-----
Icons are slightly modified versions of icons from Paul Davey's
"Umicons Volume 2" icon set. You can find his website at:
[http://mattahan.deviantart.com/art/Umicons-Volume-2-1948945](http://mattahan.deviantart.com/art/Umicons-Volume-2-1948945)


Download
--------
Latest version can always be found at:
[https://github.com/nicklan/pnmixer/downloads](https://github.com/nicklan/pnmixer/downloads)

Compilation and Install
-----------------------
Needed dependencies:
- >=gtk+-3.6	(or >=gtk+-2.22 via --without-gtk3)
- glib-2
- alsa-lib	(aka libasound on some distros)
- libX11
- libnotify	(optional, disable via --without-libnotify)
- intltool	(build-time only)
- pkg-config	(build-time only)
- autoconf	(build-time only)
- automake	(build-time only)

To install this program cd to this directory and run:

    ./autogen.sh
    make
    sudo make install

Documentation
-------------
Run:

    make doc

to build the documentation. It's in src/html. You can install
it via:

    make install-doc
