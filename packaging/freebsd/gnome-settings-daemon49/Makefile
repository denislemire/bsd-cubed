PORTNAME=	gnome-settings-daemon
PORTVERSION=	49.1
CATEGORIES=	sysutils gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	GNOME settings daemon
WWW=		https://gitlab.gnome.org/GNOME/gnome-settings-daemon

LICENSE=	GPLv2+ LGPL21+
LICENSE_COMB=	multi
LICENSE_FILE_GPLv2+ =	${WRKSRC}/COPYING
LICENSE_FILE_LGPL21+ =	${WRKSRC}/COPYING.LIB

PORTSCOUT=	limit:^49\.

RUN_DEPENDS=	bsd-cubed>=0.2.0.a2:sysutils/bsd-cubed

BUILD_DEPENDS=	gsettings-desktop-schemas>=49.1:devel/gsettings-desktop-schemas
LIB_DEPENDS=	libcanberra.so:audio/libcanberra \
		libcolord.so:graphics/colord \
		libcups.so:print/cups \
		libfontconfig.so:x11-fonts/fontconfig \
		libgeoclue-2.so:net/geoclue \
		libgeocode-glib-2.so:net/geocode-glib2 \
		libgweather-4.so:net/libgweather4 \
		liblcms2.so:graphics/lcms2 \
		libnotify.so:devel/libnotify \
		libnspr4.so:devel/nspr \
		libnss3.so:security/nss \
		libpolkit-gobject-1.so:sysutils/polkit \
		libpulse.so:audio/pulseaudio \
		libupower-glib.so:sysutils/upower \
		libp11-kit.so:security/p11-kit \
		libgcr-4.so:security/gcr

USE_XORG=	x11 xfixes
USE_GNOME=	glib20 gnomedesktop4
USES=		gettext gnome localbase:ldflags meson perl5 pkgconfig \
		python:build tar:xz xorg
USE_PERL5=	build
MESON_ARGS=	-Dalsa=false \
		-Dcups=true \
		-Dnetwork_manager=false \
		-Drfkill=false \
		-Dsmartcard=true \
		-Dusb-protection=false \
		-Dsystemd=false \
		-Dwwan=false \
		-Dx11=false \
		-Dxwayland=true
BINARY_ALIAS=	python3=${PYTHON_VERSION}

OPTIONS_DEFINE=	GUDEV
OPTIONS_DEFAULT=	GUDEV
OPTIONS_SUB=	yes

GUDEV_DESC=		GUdev device discovery
GUDEV_MESON_TRUE=	gudev
GUDEV_LIB_DEPENDS=	libgudev-1.0.so:devel/libgudev

PLIST_SUB=	VERSION=${PORTVERSION:C/^([0-9]+)\..*/\1/}

GLIB_SCHEMAS=	org.gnome.settings-daemon.enums.xml \
		org.gnome.settings-daemon.global-shortcuts.gschema.xml \
		org.gnome.settings-daemon.peripherals.gschema.xml \
		org.gnome.settings-daemon.peripherals.wacom.gschema.xml \
		org.gnome.settings-daemon.plugins.color.gschema.xml \
		org.gnome.settings-daemon.plugins.gschema.xml \
		org.gnome.settings-daemon.plugins.media-keys.gschema.xml \
		org.gnome.settings-daemon.plugins.power.gschema.xml \
		org.gnome.settings-daemon.plugins.power.gschema.xml.override \
		org.gnome.settings-daemon.plugins.sharing.gschema.xml \
		org.gnome.settings-daemon.plugins.xsettings.gschema.xml

post-install:
	${INSTALL_DATA} ${FILESDIR}/org.gnome.settings-daemon.plugins.power.gschema.xml.override \
		${STAGEDIR}${PREFIX}/share/glib-2.0/schemas/

.include <bsd.port.mk>
