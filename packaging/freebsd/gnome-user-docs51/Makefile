PORTNAME=	gnome-user-docs
PORTVERSION=	51.0
CATEGORIES=	misc gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	GNOME user documentation
WWW=		https://gitlab.gnome.org/GNOME/gnome-user-docs

LICENSE=	CC-BY-3.0
LICENSE_FILE=	${WRKSRC}/COPYING

PORTSCOUT=	limit:^51\.

BUILD_DEPENDS=	itstool:textproc/itstool

USES=		gettext-tools gnome meson tar:xz
USE_GNOME=	libxml2:build
MESON_ARGS=	-Dtests=false
NO_ARCH=	yes

.include <bsd.port.mk>
