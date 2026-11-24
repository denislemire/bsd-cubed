PORTNAME=	sushi
PORTVERSION=	51.2
CATEGORIES=	x11-fm gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Quick previewer for different kinds of files
WWW=		https://gitlab.gnome.org/GNOME/sushi

LICENSE=	GPLv2
LICENSE_FILE=	${WRKSRC}/COPYING

LIB_DEPENDS=	libwebkitgtk-6.0.so:www/webkit2-gtk@60 \
		libfribidi.so:converters/fribidi \
		libgjs.so:lang/gjs \
		libfreetype.so:print/freetype2 \
		libppsdocument-4.0.so:graphics/papers \
		libglycin-2.so:graphics/glycin \
		libepoxy.so:graphics/libepoxy \
		libharfbuzz.so:print/harfbuzz
RUN_DEPENDS=	libjxl>0:graphics/libjxl

USES=		compiler:c11 gettext-tools gnome gstreamer localbase meson \
		pkgconfig tar:xz xorg
USE_CSTD=	c11
USE_XORG=	x11
USE_GNOME=	cairo 	gdkpixbuf gtk40 libadwaita gtksourceview5 introspection:build

.include <bsd.port.mk>
