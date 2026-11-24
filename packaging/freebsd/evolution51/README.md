# Evolution 3.62.1 candidate port

This FreeBSD port builds the Evolution mail and calendar application from the GNOME 51.1 release series. It requires Evolution Data Server 3.62.1 or later and uses WebKitGTK 4.1. The base port and FreeBSD iconv patch come from FreeBSD ports revision 2072800715389ae49066946c960e6521bac37e6a.

The distinfo checksum matches the upstream release checksum. The native FreeBSD package passed file-list, stage, and installed-file checks. On an isolated Xvfb display, the installed executable reported Evolution 3.62.1. Interactive mail and calendar use await desktop validation.

https://download.gnome.org/sources/evolution/3.62/evolution-3.62.1.tar.xz
