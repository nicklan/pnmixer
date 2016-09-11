# Hacking

At first, be sure to run PNMixer from the root directory, otherwise it won't
find the data files.

	./src/pnmixer

To switch on debug messages, invoke PNMixer with the `-d` command-line option.

	./src/pnmixer -d

In order to build the documentation, be sure to have
[Doxygen](http://www.doxygen.org) and [Graphviz](htpp://www.graphviz.org)
installed. Then run the following commands to build and view the doc.

	make doc
	x-www-browser ./src/html/index.html

## Design overview

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

## Coding style

This is more or less kernel coding style. Try to match the surroundings.
For automatic code indentation we use [astyle](http://astyle.sourceforge.net/).
Please run `make indent-code` if you want to indent the whole sources.

To indent the xml ui files, we use [xmllint](http://xmlsoft.org/xmllint.html).
Please run `make indent-xml` if you want to indent the xml ui files.

## Naming conventions for signal handlers

Gtk signals are usually tied to a handler in the ui file. Therefore there's no
need to explicitly connect the signal handler in the C code, this is done
automatically when invoking `gtk_builder_connect_signals()`.

Signal handlers are named according to the following convention:
`on_<widget_name>_<signal_name>`. Please stick to it.

Signal handlers are NOT static functions, therefore names may clash in the
future when adding new signal handlers. To avoid that, either make signal
handlers static (this involves more fiddling with `gtk_builder`), or change
the naming convention.

## Comments

* comments are in doxygen format
* all functions, all data types, all macros, all typedefs... basically everything
* comments where people read them, so preferably at the definition of a function

## Good practices

* use const modifiers whenever possible, especially on function parameters
* if unsure whether to make a function static or not, make it static first
* use unsigned ints instead of signed ints whenever possible

## Translating

In order to update the po files, run the following command:

	cd po && make update-po 

Then, you need to build an archive of the project, containing the POT file,
and give it to the [Translation Project](http://translationproject.org).
The POT file (`po/pnmixer.pot`) is NOT versioned by git. 
To make it available, you need to build an archive.

	make dist

Then, on the github page of the project, create a new release, and attach
the archive. Copy the link of this archive, and send a mail to the TP with
this link. More detailed information are available on the
[maintainers](http://translationproject.org/html/maintainers.html)
page of the TP.

Furthermore, please notice that the TP will process a POT file only once,
so another submission must use a newer VERSION. Therefore, before creating
an archive, you should bump the version and tag the commit to something like
`v0.7-rc1`. It makes things cleaner.

When the translation is done, you can import the PO files with the command:

	rsync -Lrtvz translationproject.org::tp/latest/pnmixer/ po/

If you want to run PNMixer in another language, try a command such as:

	LANGUAGE=fr ./src/pnmixer

Your system might pickup the translation file installed on your system,
rather than the local one. Please take care of that.

At last, don't forget to update the translators list in the About dialog.
If new languages have beend added, update the files `Changelog`, `po/LINGUAS`,
and the variable `ALL_LINGUAS` in `configure.ac`.

## How to contribute

* [pull request on github](https://github.com/nicklan/pnmixer/pulls)
* email with pull request
* email with patch

## Tips and Tricks

* if you use vim with [youcompleteme](http://valloric.github.io/YouCompleteMe/), you can use the following [.ycm_extra_conf.py](https://gist.github.com/hasufell/0a97cc13de3ef2f061bb)
