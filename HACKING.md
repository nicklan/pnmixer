Hacking
=======

Table of Contents
-----------------

* [Getting Started](#getting-started)
* [Design Overview](#design-overview)
* [Coding Style](#coding-style)
* [Translating](#translating)
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

Translating
-----------

When a new version of PNMixer is about to be released, we need to inform the
[Translation Project](http://translationproject.org) so that they can update
the translations. The TP just needs to know where to download an up to date
archive of PNMixer. This archive must contain an up to date POT file.

The TP is considered as our upstream when it comes to the PO files. Our job
is to provide them an updated version of the POT file before each new release,
and their job is to provide us with freshly translated PO files. There should
be no exception to this workflow. We should never modify the PO files ourselves,
and we should never accept translations from another channel than the TP.

In order to update the POT file, one has to run the following command from the
`build` directory.

	make -C po update-pot
	# Then commit the new POT file

The procedure for announcing a new version of PNMixer to the TP is thoroughly
described on the [maintainters](http://translationproject.org/html/maintainers.html)
page. In a nutshell, send a mail to <coordinator@translationproject.org>, use
`pnmixer-<version>.pot` as the subject, and include in the body the url of the
new PNMixer archive.

So, let's sum up the steps to follow to update the translations.

- update the pot file, commit.
- tag a new version (don't forget to bump the version where need be), commit.
- push the changes and the tags.
- have a look on the GitHub release page, copy the url of the `tar.gz` archive.
- send a mail to the TP with this url.

Please note that the TP will process a POT file only once, so another submission
must use a newer version.

To know about the current translation status, visit the [pnmixer textual domain
](https://translationproject.org/domain/pnmixer.html) page on the TP.
Whenever translations are updated, we (elboulangero) get notified by email.

The latest translations can be imported back in PNMixer at any time, using
the following command.

	rsync -Lrtvz translationproject.org::tp/latest/pnmixer/ po/
	# Then commit the new PO files

After importing translations, don't forget to:

- update the translators list in the "About" dialog.
- update the `ChangeLog`.
- if new languages have been added, add them to the file `po/LINGUAS`.

If you're curious to see how PNMixer looks like in another language, it's easy
to check it out.

	LANGUAGE=fr ./src/pnmixer

Your system might pickup the translation file installed on your system,
rather than the local one. Please take care of that.

How to Contribute
-----------------

* [pull request on github](https://github.com/nicklan/pnmixer/pulls)
* email with pull request
* email with patch

Tips and Tricks
---------------

* if you use vim with [youcompleteme](http://valloric.github.io/YouCompleteMe/), you can use the following [.ycm_extra_conf.py](https://gist.github.com/hasufell/0a97cc13de3ef2f061bb)
