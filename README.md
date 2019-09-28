DeforaOS libApp
===============

About libApp
------------

libApp is a message-passing framework. It is designed to become the core
component for the DeforaOS Project, providing transparent and protocol-agnostic
network computing capabilities. Regardless, libApp is intended to be portable
and work on any system, using any POSIX-compliant runtime as fallback if
available.


Dependencies for libApp
-----------------------

libApp depends on the following software components to build:
- pkg-config from the freedesktop software collection, as found in most
  software distributions already; otherwise at
  <https://www.freedesktop.org/wiki/Software/pkg-config/> (or a compatible
  replacement)
- libSystem from the DeforaOS Project, as found at
  <https://www.defora.org/os/project/27/libSystem>
- libMarshall from the DeforaOS Project, as found at
  <https://www.defora.org/os/project/4400/libMarshall>
- configure from the DeforaOS Project, likewise found at
  <https://www.defora.org/os/project/16/configure>


Configuring libApp
------------------

Before being able to build libApp, it is necessary to generate Makefiles with
`configure(1)`. It should be enough to run this tool as follows:

    $ configure

Please refer to the documentation of DeforaOS configure for further
instructions.


Compiling libApp
----------------

It is then possible to build the project with `make(1)` as usual. The following
command should therefore be enough:

    $ make

To install libApp in a dedicated directory, like `/path/to/libApp`:

    $ make PREFIX="/path/to/libApp" install
