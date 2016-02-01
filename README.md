DeforaOS libApp
===============

About libApp
------------

libApp is a message-passing framework. It is designed to become the core
component for the DeforaOS Project, providing transparent and protocol-agnostic
network computing capabilities. Regardless, libApp is intended to be portable
and work on any system, using any POSIX-compliant runtime as fallback if
available.


Compiling libApp
----------------

Before being able to build libApp, it is necessary to generate Makefiles with
`configure(1)` from the DeforaOS configure project, found at
<http://www.defora.org/os/project/16/configure>. It should be enough to run
this tool as follows:

    $ configure

Please refer to the documentation of DeforaOS configure for further
instructions.

It is then possible to build the project with `make(1)` as usual. The following
command should therefore be enough:

    $ make

To install libApp in a dedicated directory, like `/path/to/libApp`:

    $ make PREFIX="/path/to/libApp" install
