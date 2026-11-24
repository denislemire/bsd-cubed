PORTNAME=	gnome-autoar
DISTVERSION=	0.5.2
CATEGORIES=	archivers
MASTER_SITES=	GNOME
DISTFILES=	${DISTNAME}${EXTRACT_SUFX} gnome-autoar-0.4.5.tar.xz
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Glib wrapper around libarchives
WWW=		https://gitlab.gnome.org/GNOME/gnome-autoar

LICENSE=	LGPL21
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		cpe gettext-runtime gnome libarchive meson pkgconfig \
		tar:xz vala:build

CPE_VENDOR=	gnome
USE_GNOME=	cairo gdkpixbuf gtk30 introspection:build

OPTIONS_DEFINE=	DOCS

DOCS_MESON_TRUE=	docs
DOCS_BUILD_DEPENDS=	gi-docgen:textproc/py-gi-docgen

# Evolution still uses the GTK 3 chooser removed in 0.5.
COMPAT_SRC=	${WRKDIR}/gnome-autoar-0.4.5
COMPAT_STAGE=	${WRKDIR}/compat-stage

post-configure:
	cd ${COMPAT_SRC} && ${SETENV} ${CONFIGURE_ENV} meson setup ${MESON_BUILD_DIR} --prefix=${PREFIX} --libdir=lib -Dgtk_doc=false

post-build:
	${SETENV} ${MAKE_ENV} ninja -C ${COMPAT_SRC}/${MESON_BUILD_DIR} -j ${MAKE_JOBS_NUMBER}

post-install:
	${SETENV} ${MAKE_ENV} DESTDIR=${COMPAT_STAGE} meson install --no-rebuild -C ${COMPAT_SRC}/${MESON_BUILD_DIR}
	cd ${COMPAT_STAGE}${PREFIX} && ${TAR} -cf - -T ${FILESDIR}/compat-plist | ${TAR} -xf - -C ${STAGEDIR}${PREFIX}

.include <bsd.port.mk>
