# Hacking

To view the documentation, run
```sh
make doc
```
and browse it via `./src/html/index.html`.

## Coding style

This is more or less kernel coding style. Try to match the surroundings.
You can also use the tool [astyle](http://astyle.sourceforge.net/) and
execute the following command to fix the source files:
```sh
astyle --style=linux --indent=tab=8 --align-pointer=name src/*.[ch]
```

## Comments

* comments are in doxygen format
* all functions, all data types, all macros, all typedefs... basically everything
* comments where people read them, so preferably at the definition of a function

## Good practices

* use const modifiers whenever possible, especially on function parameters
* if unsure whether to make a function static or not, make it static first
* use unsigned ints instead of signed ints whenever possible

## How to contribute

* [pull request on github](https://github.com/nicklan/pnmixer/pulls)
* email with pull request
* email with patch

## Tips and Tricks

* if you use vim with [youcompleteme](http://valloric.github.io/YouCompleteMe/), you can use the following [.ycm_extra_conf.py](https://gist.github.com/hasufell/0a97cc13de3ef2f061bb)
