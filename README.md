syncp(1) -- Synchronize cached writes to persistent storage, but with progress
==============================================================================

## SYNOPSIS

`syncp` [<var>OPTION</var>...] [<var>FILE</var>...]

## DESCRIPTION

Synchronize cached writes to persistent storage

If one or more files are specified, sync only them,
or their containing file systems.

When syncing, display size of remaining data to sync.

## OPTIONS

* `-d`, `--data`:
    sync only file data, no unneeded metadata

* `-f`, `--file-system`:
    sync the file systems that contain the files

* `-t`, `--timeout` <var>N</var>:
    timeout (in seconds) when to exit if sync still not finished

* `-p`, `--period` <var>N</var>:
    period (in seconds) to check buffers size

* `-h`, `--help`:
    display this help and exit

* `-v`, `--version`:
    output version information and exit

## BUILDING FROM SOURCE

Source code is available in GitHub repository:
https://github.com/makise-homura/syncp

You will need meson(1), gettext(1), ninja(1), and preferably
git(1) to determine version, and ronn(1) to generate documentation.

```
git clone https://github.com/makise-homura/syncp
cd syncp
mkdir build
cd build
meson ..
ninja
sudo ninja install
```

You may add `-Dprefix=`<var>INSTALL PREFIX</var> to `meson` invocation;
usually it is `/usr`. If not specified, defaults to `/usr/local`.

To update `README.md`, also run `ninja README.md`,
then copy resulting file to source tree.

If you're interested in writing your own translations, refer to
manual of `i18n` module of meson(1). Targets of subject are:

* To update pot files, run `ninja syncp-pot`.

* To update po files, run `ninja syncp-update-po`.

## AUTHOR

This software is based on sync(1) from GNU coreutils,
which is written by Jim Meyering and Giuseppe Scrivano.

Progress part is written by Igor Molchanov (aka makise-homura).

## REPORTING BUGS

Feel free to open issues and pull requests in GitHub repository:
https://github.com/makise-homura/syncp

## SEE ALSO

sync(1), fdatasync(2), fsync(2), sync(2), syncfs(2)


[SYNOPSIS]: #SYNOPSIS "SYNOPSIS"
[DESCRIPTION]: #DESCRIPTION "DESCRIPTION"
[OPTIONS]: #OPTIONS "OPTIONS"
[BUILDING FROM SOURCE]: #BUILDING-FROM-SOURCE "BUILDING FROM SOURCE"
[AUTHOR]: #AUTHOR "AUTHOR"
[REPORTING BUGS]: #REPORTING-BUGS "REPORTING BUGS"
[SEE ALSO]: #SEE-ALSO "SEE ALSO"


[syncp(1)]: syncp.1.html
