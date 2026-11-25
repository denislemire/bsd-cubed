PORTNAME=	gdm
DISTVERSION=	51.0
PORTREVISION=	2
CATEGORIES=	x11 gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	denis@lemire.name
COMMENT=	GNOME display manager with native FreeBSD seat and session support
WWW=		https://gitlab.gnome.org/GNOME/gdm

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

BUILD_DEPENDS=	bsd-cubed>=0.2.0.a3_4:sysutils/bsd-cubed
LIB_DEPENDS=	libaccountsservice.so:sysutils/accountsservice \
		libgudev-1.0.so:devel/libgudev \
		libjson-glib-1.0.so:devel/json-glib \
		libpolkit-gobject-1.so:sysutils/polkit
RUN_DEPENDS=	bsd-cubed>=0.2.0.a3_4:sysutils/bsd-cubed \
		gnome-session>=51:x11/gnome-session \
		gnome-shell>=51:x11/gnome-shell \
		gnome-keyring>=51:security/gnome-keyring

USES=		gettext gnome meson pkgconfig tar:xz
USE_GNOME=	dconf glib20 introspection:build
USE_LDCONFIG=	yes
USE_RC_SUBR=	gdm
USERS=		gdm
GROUPS=		gdm
GLIB_SCHEMAS=	org.gnome.login-screen.gschema.xml
MESON_ARGS=	-Dlogind-provider=bsd3 -Dinitial-vt=9 -Dx11-support=false \
		-Dsystemd-journal=false -Dsystemdsystemunitdir=no -Dsystemduserunitdir=no \
		-Ddefault-pam-config=none -Dplymouth=disabled -Dselinux=disabled \
		-Dlibaudit=disabled -Drun-dir=/var/run/gdm -Dlog-dir=/var/log/gdm \
		-Dworking-dir=/var/db/gdm51 -Dpid-file=/var/run/gdm.pid \
		-Dprivate-dbus-dir=/var/run/gdm -Ddbus-sys=${PREFIX}/etc/dbus-1/system.d
SUB_FILES=	bsd3-gdm-greeter bsd3-gdm-password bsd3-gdm-launch \
		greeter gnome-session-manager gnome-session-shutdown gnome-shell

post-patch:
	${CP} ${FILESDIR}/gdm-static-user-store.c ${WRKSRC}/daemon/

post-install:
	${MV} ${STAGEDIR}${PREFIX}/etc/gdm/custom.conf ${STAGEDIR}${PREFIX}/etc/gdm/custom.conf.sample
	${MKDIR} ${STAGEDIR}${PREFIX}/etc/pam.d ${STAGEDIR}${PREFIX}/share/gdm/bsd3-services
.for svc in bsd3-gdm-launch bsd3-gdm-password
	${INSTALL_DATA} ${WRKDIR}/${svc} ${STAGEDIR}${PREFIX}/etc/pam.d/${svc}.sample
.endfor
.for svc in greeter gnome-session-manager gnome-session-shutdown gnome-shell
	${INSTALL_DATA} ${WRKDIR}/${svc} ${STAGEDIR}${PREFIX}/share/gdm/bsd3-services/
.endfor
	${INSTALL_SCRIPT} ${WRKDIR}/bsd3-gdm-greeter ${STAGEDIR}${PREFIX}/libexec/
	${INSTALL_SCRIPT} ${FILESDIR}/bsd3-gdm-check-account ${STAGEDIR}${PREFIX}/libexec/
	${MKDIR} ${STAGEDIR}${PREFIX}/etc/gdm/home ${STAGEDIR}/var/db/gdm51 ${STAGEDIR}/var/log/gdm

.include <bsd.port.mk>
