[![GitHub release](https://img.shields.io/github/release/nicklan/pnmixer.svg)](https://github.com/nicklan/pnmixer/releases)
[![Build Status](https://travis-ci.org/nicklan/pnmixer.svg?branch=master)](https://travis-ci.org/nicklan/pnmixer)
[![license](https://img.shields.io/github/license/nicklan/pnmixer.svg)](COPYING)

PNMixer
=======

Table of Contents
-----------------
* [About](#about)
* [Download](#download)
* [Compilation and Install](#compilation-and-install)
	* [Distro packages](#distro-packages)
	* [Manual](#manual)
* [Icons](#icons)
* [Translation](#translation)
* [TODO/Help wanted](#todohelp-wanted)
* [Known Bugs](#known-bugs)

About
-----
PNMixer is a simple mixer application designed to run in your system
tray. It integrates nicely into desktop environments that don't have
a panel that supports applets and therefore can't run a mixer applet.
In particular it's been used quite a lot with [fbpanel][] and [tint2][],
but should run fine in any system tray.

PNMixer is designed to work on systems that use ALSA for sound management.
Any other sound driver like OSS or FFADO are currently not supported
(patches welcome). There is no *official* PulseAudio support at the moment,
but it seems that PNMixer behaves quite well anyway when PA is running.
Feel free to try and to give some feedback.

PNMixer is a fork of [OBMixer][] with a number of additions. These include:

- Volume adjustment with the scroll wheel
- Select which ALSA device and channel to use
- Detect disconnect from sound system and re-connect if requested
- Bind and use HotKeys for volume control
- Textual display of volume level in popup window
- Continuous volume adjustment when dragging the slider (not just when you let go)
- Draw a volume level onto system tray icon
- Use system icon theme for icons and use mute/low/medium/high
  volume icons
- Configurable middle click action
- Preferences for:
	- volume text display
	- volume text position
	- icon theme
	- amount to adjust per scroll
	- middle click action
	- drawing of volume level on tray icon

Source and so on are at: <https://github.com/nicklan/pnmixer>

[fbpanel]: https://github.com/aanatoly/fbpanel
[tint2]:   https://gitlab.com/o9000/tint2
[obmixer]: http://jpegserv.com/?page_id=282

Download
--------
Latest version can always be found at: <https://github.com/nicklan/pnmixer/releases>

### Verifying a release tarball

Releases can be verified via [signify](https://github.com/aperezdc/signify).
Get the [pubkey](https://github.com/nicklan/pnmixer/wiki/signify_keys/pnmixer_signify.pub) and [verify](https://github.com/nicklan/pnmixer/wiki/signify_keys/pnmixer_signify.pub.asc) it against the GPG key
`0x511B62C09D50CD28`.
Download the static tarball and `SHA256` as well as `SHA256.sig` into the
same directory, then run:

```sh
signify -V -p pnmixer_signify.pub -m SHA256
```

Alternatively you can just GPG-verify the tarball with the detached
signature.

Compilation and Install
-----------------------

The best way to install most software is via your distribution. Only install
manually if your distribution does not provide a package.

### Distro packages

* [Debian](https://packages.debian.org/search?keywords=pnmixer&searchon=names&suite=all&section=all)
* [Gentoo](https://packages.gentoo.org/packages/media-sound/pnmixer)
* [Exherbo](https://git.exherbo.org/summer/packages/media-sound/pnmixer/index.html)
* [![AUR](https://img.shields.io/aur/version/pnmixer.svg)](https://aur.archlinux.org/packages/pnmixer/)

### Manual

CMake Options:
- `WITH_GTK3`: Use Gtk3 as toolkit (default on)
- `WITH_LIBNOTIFY`: Enable sending of notifications (default on)
- `ENABLE_NLS`: Enable building of translations (default on)
- `BUILD_DOCUMENTATION`: Use Doxygen to create the HTML based API documentation (default off)

First, make sure you have the required __dependencies__:
- build:
	- cmake
	- doxygen (when building documentation)
	- graphviz (when building documentation)
	- gettext (when building translations)
	- pkg-config
- build+runtime:
	- alsa-lib (aka libasound on some distros)
	- glib-2
	- >=gtk+-3.12 (or >=gtk+-2.24 when disabling gtk3)
	- libnotify (when enabling notifications)
	- libX11
- runtime suggestions (PNMixer can use a full mixer):
	- alsamixergui
	- gnome-alsamixer
	- xfce4-mixer

To __install__ this program cd to this directory and run:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    make install

Icons
-----
Icons are a slightly modified versions of the icons from Paul Davey's
"Umicons Volume 2" icon set. You can find his website at:
<http://mattahan.deviantart.com/art/Umicons-Volume-2-1948945>

Translation
-----------
PNMixer is translated through the [Translation Project](https://translationproject.org/).
If you wish to make or update a translation for PNMixer, please get in touch
with the relevant team on the TP. Solo translations won't be accepted.

You can visit the PNMixer page on the TP at
<https://translationproject.org/domain/pnmixer.html>.

TODO/Help wanted
---------------

- [Move away from deprecated GtkStatusIcon?](https://github.com/nicklan/pnmixer/issues/81)

- **PulseAudio support:** this needs to be implemented from scratch and **will likely not happen**. The structure of how PulseAudio works is fundamentally different from Alsa and doesn't seem to play well with the current design of PNMixer. Either we'd need two completely divergent codepaths or try really hard to abstract over both Alsa and PulseAudio, which in itself is kind of weird (since PulseAudio is supposed to abstract over Alsa etc.).

Known Bugs/Glitches
-------------------

- On panel sizes of 21 and 22 pixels, the volume meter offset can be messed up (gtk3 only). This seems to be a gtk3 bug, not a PNMixer one. Also see [issue 136](https://github.com/nicklan/pnmixer/issues/136).

- volume slider popup window overlaps desktop panel, see [issue 71](https://github.com/nicklan/pnmixer/issues/71)

- There are various problems with PulseAudio, since there is **no PulseAudio** backend. One specific issue is the unmute functionality misbehaving, also see [issue 70](https://github.com/nicklan/pnmixer/issues/70).

You can also skim through the [issue tracker](https://github.com/nicklan/pnmixer/issues?q=is%3Aissue+is%3Aopen+label%3Abug).

