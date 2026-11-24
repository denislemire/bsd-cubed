PORTNAME=	gnome-qr
DISTVERSION=	51.0
CATEGORIES=	graphics gnome
MASTER_SITES=	GNOME/sources/gnome-desktop
DISTNAME=	gnome-desktop-${DISTVERSION}
DIST_SUBDIR=	gnome

MAINTAINER=	denis@lemire.name
COMMENT=	GNOME QR code library and introspection data for GNOME Shell
WWW=		https://gitlab.gnome.org/GNOME/gnome-desktop

LICENSE=	GPLv3+ MIT
LICENSE_COMB=	multi

USES=		gnome meson pkgconfig tar:xz
USE_GNOME=	glib20 introspection:build
USE_LDCONFIG=	yes

# Build the upstream QR component alongside the installed GNOME Desktop ABI.
post-patch:
	${CP} ${FILESDIR}/meson.build ${WRKSRC}/meson.build

.include <bsd.port.mk>
