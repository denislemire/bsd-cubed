PORTNAME= gnome-session
DISTVERSION= 49.3
CATEGORIES= x11 gnome
MASTER_SITES= GNOME
DIST_SUBDIR= gnome

MAINTAINER= denis@lemire.name
COMMENT= GNOME49 session with native BSD3/ConsoleKit service lifecycle
WWW= https://gitlab.gnome.org/GNOME/gnome-session

LICENSE= GPLv2+
LICENSE_FILE= ${WRKSRC}/COPYING

BUILD_DEPENDS= ${LOCALBASE}/libdata/pkgconfig/bsd3-user-session.pc:sysutils/bsd-cubed
RUN_DEPENDS= bsd-cubed>=0.2.0.a1:sysutils/bsd-cubed \
             gnome-settings-daemon>=47.2:sysutils/gnome-settings-daemon \
             gnome-shell>=49:x11/gnome-shell \
             dbus-daemon:devel/dbus

USES= compiler:c11 gettext gnome localbase:ldflags meson pkgconfig tar:xz
USE_GNOME= glib20 gnomedesktop4 gtk40
MESON_ARGS= -Dbsd3=true -Dx11=false -Ddocbook=false -Dman=false
GLIB_SCHEMAS= org.gnome.SessionManager.gschema.xml

post-patch:
	${PATCH} -d ${WRKSRC} -p1 < ${.CURDIR}/native-manager.patch
	${CP} ${LOCALBASE}/share/bsd-cubed/gnome49/gnome49-system.c ${WRKSRC}/gnome-session/
	${CP} ${LOCALBASE}/share/bsd-cubed/gnome49/gnome49-system.h ${WRKSRC}/gnome-session/

post-install:
	${LN} -s ${PREFIX}/libexec/bsd3-gnome49-session ${STAGEDIR}${PREFIX}/bin/gnome-session

.include <bsd.port.mk>
