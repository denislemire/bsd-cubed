PORTNAME=	gnome-desktop
PORTVERSION=	51.0
CATEGORIES=	x11 gnome
MASTER_SITES=	GNOME
DISTFILES=	${DISTNAME}${EXTRACT_SUFX} gnome-desktop-44.5.tar.xz
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Additional UI API for GNOME
WWW=		https://gitlab.gnome.org/GNOME/gnome-desktop

LICENSE=	GPLv2+ LGPL20+
LICENSE_COMB=	multi
LICENSE_FILE_GPLv2+ =	${WRKSRC}/COPYING
LICENSE_FILE_LGPL20+ =	${WRKSRC}/COPYING.LIB

BUILD_DEPENDS=	iso-codes>0:misc/iso-codes \
		gsettings-desktop-schemas>0:devel/gsettings-desktop-schemas \
		xkeyboard-config>0:x11/xkeyboard-config
LIB_DEPENDS=	libgraphene-1.0.so:graphics/graphene \
		libgnome-qr-4.so:graphics/gnome-qr \
		libxkbregistry.so:x11/libxkbcommon \
		libudev.so:devel/libudev-devd
RUN_DEPENDS=	iso-codes>0:misc/iso-codes \
		gsettings-desktop-schemas>0:devel/gsettings-desktop-schemas \
		cantarell-fonts>0:x11-fonts/cantarell-fonts \
		source-code-pro-ttf>0:x11-fonts/source-code-pro-ttf \
		xkeyboard-config>0:x11/xkeyboard-config

USES=		cpe gettext-tools gnome localbase:ldflags meson \
		pkgconfig tar:xz xorg
CPE_VENDOR=	gnome
USE_GNOME=	cairo gdkpixbuf gtk30 gtk40 introspection:build
USE_LDCONFIG=	yes
USE_XORG=	x11
MESON_ARGS=	-Dudev=enabled \
		-Dsystemd=disabled \
		-Ddebug_tools=false \
		-Ddesktop_docs=false

# Keep the previous desktop and RandR ABI available for existing consumers.
COMPAT_SRC=	${WRKDIR}/gnome-desktop-44.5
COMPAT_STAGE=	${WRKDIR}/compat-stage

post-patch:
	cd ${COMPAT_SRC} && ${PATCH} -s -p0 < ${FILESDIR}/patch-libgnome-desktop_gnome-desktop-thumbnail-script.c

post-configure:
	cd ${COMPAT_SRC} && ${SETENV} ${CONFIGURE_ENV} meson ${CONFIGURE_ARGS}

post-build:
	${SETENV} ${MAKE_ENV} ninja -C ${COMPAT_SRC}/${MESON_BUILD_DIR} -j ${MAKE_JOBS_NUMBER}

post-install:
	${SETENV} ${MAKE_ENV} DESTDIR=${COMPAT_STAGE} meson install --no-rebuild -C ${COMPAT_SRC}/${MESON_BUILD_DIR}
	cd ${COMPAT_STAGE}${PREFIX} && ${TAR} -cf - -T ${FILESDIR}/compat-plist | ${TAR} -xf - -C ${STAGEDIR}${PREFIX}
	cd ${STAGEDIR}${PREFIX} && ${XARGS} ${RM} < ${FILESDIR}/qr-plist
	${RMDIR} ${STAGEDIR}${PREFIX}/include/gnome-desktop-4.0/gnome-qr

.include <bsd.port.mk>
