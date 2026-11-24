PORTNAME=	gdm
DISTVERSION=	51.0
CATEGORIES=	x11 gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	denis@lemire.name
COMMENT=	GNOME Display Manager with native FreeBSD session support
WWW=		https://gitlab.gnome.org/GNOME/gdm

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

BUILD_DEPENDS=	${LOCALBASE}/libdata/pkgconfig/bsd3-login.pc:sysutils/bsd-cubed \
		itstool:textproc/itstool
LIB_DEPENDS=	libaccountsservice.so:sysutils/accountsservice \
		libgudev-1.0.so:devel/libgudev \
		libpolkit-gobject-1.so:sysutils/polkit
RUN_DEPENDS=	bsd-cubed>=0.2.0.a3_2:sysutils/bsd-cubed \
		gnome-shell>=51:x11/gnome-shell

USES=		cpe gettext gnome localbase:ldflags meson pkgconfig tar:xz
CPE_VENDOR=	gnome
USE_GNOME=	dconf glib20 introspection:build libxml2:build
MESON_ARGS=	-Dlogind-provider=bsd-cubed \
		-Dsystemdsystemunitdir=no -Dsystemduserunitdir=no \
		-Dsystemd-journal=false -Dx11-support=false \
		-Dplymouth=disabled -Dselinux=disabled -Ddefault-pam-config=none

.include <bsd.port.mk>
