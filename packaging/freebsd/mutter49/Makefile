PORTNAME=	mutter
PORTVERSION=	49.8
PORTREVISION=	1
CATEGORIES=	x11-wm
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Window and compositing manager based on Clutter
WWW=		https://gnome.pages.gitlab.gnome.org/mutter/

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

BUILD_DEPENDS=	${PYTHON_PKGNAMEPREFIX}docutils>0:textproc/py-docutils@${PY_FLAVOR} \
		${LOCALBASE}/libdata/pkgconfig/bsd3-login.pc:sysutils/bsd-cubed \
		evdev-proto>0:devel/evdev-proto \
		gnome-settings-daemon>0:sysutils/gnome-settings-daemon \
		gsettings-desktop-schemas>0:devel/gsettings-desktop-schemas \
		xkeyboard-config>0:x11/xkeyboard-config \
		dbus>0:devel/dbus
LIB_DEPENDS=	libudev.so:devel/libudev-devd \
		libcanberra.so:audio/libcanberra \
		libgudev-1.0.so:devel/libgudev \
		libdrm.so:graphics/libdrm \
		libinput.so:x11/libinput \
		libfribidi.so:converters/fribidi \
		libgraphene-1.0.so:graphics/graphene \
		libpipewire-0.3.so:multimedia/pipewire \
		libstartup-notification-1.so:x11/startup-notification \
		libxkbcommon.so:x11/libxkbcommon \
		liblcms2.so:graphics/lcms2 \
		libcolord.so:graphics/colord \
		libharfbuzz.so:print/harfbuzz \
		libei.so:x11/libei \
		libdisplay-info.so:sysutils/libdisplay-info
RUN_DEPENDS=	${PYTHON_PKGNAMEPREFIX}dbus>0:devel/py-dbus@${PY_FLAVOR} \
		bsd-cubed>=0.1.0.b3:sysutils/bsd-cubed \
		gnome-settings-daemon>0:sysutils/gnome-settings-daemon \
		gsettings-desktop-schemas>0:devel/gsettings-desktop-schemas \
		xkeyboard-config>0:x11/xkeyboard-config

USES=		compiler:c++0x gettext-tools gl gnome localbase meson \
		pkgconfig python:build,run shebangfix tar:xz xorg
USE_GL=		egl gbm glesv2
USE_GNOME=	atk cairo gdkpixbuf gnomedesktop4 gtk40 introspection:build pygobject3
USE_XORG=	ice pixman sm x11 xcb xcomposite xcursor xdamage xext \
		xfixes xi xinerama xkbfile xrandr xrender xtst
USE_LDCONFIG=	yes
SHEBANG_FILES=	tools/gdctl tools/gnome-service-client \
		src/backends/native/gen-default-modes.py

PORTSCOUT=	limit:^49\.

CFLAGS+=	-Wno-unknown-warning-option -Wno-format-nonliteral
CPPFLAGS+=	-fno-omit-frame-pointer
MESON_ARGS=	-Dbash_completion=false \
		-Ddevkit=disabled \
		-Dlibwacom=false \
		-Dnative_backend=true \
		-Dprofiler=false \
		-Dlogind=true \
		-Dtests=disabled \
		-Dcogl_tests=false \
		-Dclutter_tests=false \
		-Dmutter_tests=false \
		-Dinstalled_tests=false

GLIB_SCHEMAS=	org.gnome.mutter.gschema.xml \
		org.gnome.mutter.wayland.gschema.xml

# The GNOME49 native FreeBSD package is explicitly Wayland, with Xwayland apps.
BUILD_DEPENDS+=	wayland-protocols>0:graphics/wayland-protocols \
		xwayland>0:x11-servers/xwayland
LIB_DEPENDS+=	libwayland-server.so:graphics/wayland
RUN_DEPENDS+=	xwayland>0:x11-servers/xwayland
USE_XORG+=	xau
MESON_ARGS+=	-Dwayland=true -Dxwayland=true -Dx11=false

.include <bsd.port.mk>
