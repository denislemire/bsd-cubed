PORTNAME=	loupe
DISTVERSION=	51.0
CATEGORIES=	graphics gnome
MASTER_SITES=	GNOME
DISTFILES=	${DISTNAME}${EXTRACT_SUFX}
DIST_SUBDIR=	gnome

MAINTAINER=	denis@lemire.name
COMMENT=	GNOME image viewer
WWW=		https://apps.gnome.org/Loupe/

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING.md

BUILD_DEPENDS=	itstool:textproc/itstool
LIB_DEPENDS=	libgraphene-1.0.so:graphics/graphene \
		libgweather-4.so:net/libgweather4
USES=		cargo desktop-file-utils gettext gnome meson pkgconfig tar:xz
USE_GNOME=	cairo glib20 gtk40 libadwaita
GLIB_SCHEMAS=	org.gnome.Loupe.gschema.xml
CARGO_BUILD=	no
CARGO_INSTALL=	no
CARGO_TEST=	no
MAKE_ENV=	${CARGO_ENV}

.include <bsd.port.mk>
