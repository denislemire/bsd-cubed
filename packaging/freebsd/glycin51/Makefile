PORTNAME=	glycin
DISTVERSION=	2.2.3
CATEGORIES=	graphics gnome
MASTER_SITES=	GNOME
DISTFILES=	${DISTNAME}${EXTRACT_SUFX}
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Image loading and metadata libraries for GNOME
WWW=		https://gitlab.gnome.org/GNOME/glycin

LICENSE=	LGPL21+ MPL20
LICENSE_COMB=	dual

USES=		cargo gnome meson pkgconfig tar:xz vala:build
USE_GNOME=	glib20 gtk40 introspection:build
USE_LDCONFIG=	yes

# The release selects glycin-builtin on FreeBSD through its Cargo manifest.
MESON_ARGS=	-Dglycin-loaders=false -Dglycin-thumbnailer=false \
		-Dlto=false -Dtests=false -Dcapi_docs=false -Dpython_tests=false
CARGO_BUILD=	no
CARGO_INSTALL=	no
CARGO_TEST=	no
MAKE_ENV=	${CARGO_ENV}

.include <bsd.port.mk>
