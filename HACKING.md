Hacking
=======

Table of Contents
-----------------

* [Getting Started](#getting-started)
* [Design Overview](#design-overview)
* [Coding Style](#coding-style)
* [How to Contribute](#how-to-contribute)
* [Tips and Tricks](#tips-and-tricks)

Getting Started
---------------

At first, be sure to run PNMixer from the root directory, otherwise it won't
find the data files.

	cd build
	./src/pnmixer

To switch on debug messages, invoke PNMixer with the `-d` command-line option.

	./src/pnmixer -d

To run PNMixer in another language, use the `LANGUAGE` environment variable.
Beware that the language files used will (probably) be the ones installed on
your system, not the ones present in the source.

	LANGUAGE=fr ./src/pnmixer

In order to build the documentation, be sure to have
[Doxygen](http://www.doxygen.org) and [Graphviz](htpp://www.graphviz.org)
installed and have configured `BUILD_DOCUMENTATION=ON` in cmake.
Then run the following command to view the doc.

	x-www-browser ./build/src/html/index.html

Design Overview
---------------

The lowest level part of the code is the sound backend. Only Alsa is supported
at the moment, but more backends may be added in the future.

The backend is hidden behind a frontend, defined in `audio.c`. Only `audio.c`
deals with audio backends. This means that the whole of the code is blissfully
ignorant of the audio backend in use.

`audio.c` is also in charge of emitting signals whenever a change happens.
This means that PNMixer design is quite *signal-oriented*, so to say.

The ui code is nothing fancy. Each ui element...

* is defined in a single file
* strives to be standalone
* accesses the sound system with function calls
* listens to signals from the audio subsystem to update its appearance

There's something you should keep in mind. Audio on a computer is a shared
resource. PNMixer isn't the only one that can change it.
At any moment the audio volume may be modified by someone else,
and we must update the ui accordingly. So listening to changes from
the audio subsystem (and therefore having a *signal-oriented* design)
is the most obvious solution to solve that problem.

Coding Style
------------

This is more or less kernel coding style. Try to match the surroundings.
For automatic code indentation we use [astyle](http://astyle.sourceforge.net/).
Please run `make indent-code` if you want to indent the whole sources.

To indent the xml ui files, we use [xmllint](http://xmlsoft.org/xmllint.html).
Please run `make indent-xml` if you want to indent the xml ui files.

### Naming conventions for signal handlers

Gtk signals are usually tied to a handler in the ui file. Therefore there's no
need to explicitly connect the signal handler in the C code, this is done
automatically when invoking `gtk_builder_connect_signals()`.

Signal handlers are named according to the following convention:
`on_<widget_name>_<signal_name>`. Please stick to it.

Signal handlers are NOT static functions, therefore names may clash in the
future when adding new signal handlers. To avoid that, either make signal
handlers static (this involves more fiddling with `gtk_builder`), or change
the naming convention.

### Comments

* comments are in doxygen format
* all functions, all data types, all macros, all typedefs... basically everything
* comments where people read them, so preferably at the definition of a function

### Good practices

* use const modifiers whenever possible, especially on function parameters
* if unsure whether to make a function static or not, make it static first
* use unsigned ints instead of signed ints whenever possible

How to Contribute
-----------------

* [pull request on github](https://github.com/nicklan/pnmixer/pulls)
* email with pull request
* email with patch

Tips and Tricks
---------------

* if you use vim with [youcompleteme](http://valloric.github.io/YouCompleteMe/), you can use the provided project `.ycm_extra_conf.py`
* if you use vim with [ALE](https://github.com/w0rp/ale) you can use the provided local `_vimrc_local.vim` to set gcc/clang cflags
	- you might have to install [local_vimrc](https://github.com/LucHermitte/local_vimrc)
	- use `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=yes`, because it generates [compile_commands.json](https://clang.llvm.org/docs/JSONCompilationDatabase.html), which can be used by ALEs `clangtidy` and `cppcheck` linters
