PORTNAME=	gnome-weather
PORTVERSION=	50.0
CATEGORIES=	deskutils gnome
MASTER_SITES=	GNOME:weather https://registry.npmjs.org/typescript/-/:ts
DISTFILES=	${DISTNAME}${EXTRACT_SUFX}:weather typescript-5.9.3.tgz:ts
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Weather application for GNOME
WWW=		https://apps.gnome.org/Weather/

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING.md

PORTSCOUT=	limit:^50\.

LIB_DEPENDS=	libgeoclue-2.so:net/geoclue \
		libgjs.so:lang/gjs \
		libgweather-4.so:net/libgweather4

USES=		desktop-file-utils gettext gnome localbase meson nodejs:build pkgconfig \
		tar:xz
USE_GNOME=	glib20 gtk40 introspection:build libadwaita
USE_LDCONFIG=	yes

GLIB_SCHEMAS=	org.gnome.Weather.gschema.xml

BINARY_ALIAS=	tsc=${WRKDIR}/package/bin/tsc

.include <bsd.port.mk>
