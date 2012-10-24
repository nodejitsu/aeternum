# Aeternum

## A process monitor in libuv

`aeternum` is a simple process monitor, implemented with `libuv`.  It is
designed for simplicity and stability.  It is still a work in progress.

## How to Use

`aeternum` will currently work best on either SunOS or BSD-derivative operating
systems.  To compile, simply run `make` from the project root.

Once compiled, usage is simple:

     ./aeternum start -o outputfile -- ./otherprogram

The `--` option is used to separate `aeternum` arguments from any arguments
passed to the child process.

### Options

 - `start`: Tells `aeternum` to background the process.  Must be the first argument.
 - `-o`: File to redirect `stdout` to.  If no separate file is provided for
   `stderr`, the same file will be used for both.
 - `-e`: File to redirect `stderr` to. (optional)
 - `-p`: pidfile - the name of the pidfile to use.  If no relative or absolute
   path is provided, `$HOME/.aeternum/` will be the base path used.
 - `-i`: inputfile - If used, the file here will be available as the `stdin` of
   the child process.
