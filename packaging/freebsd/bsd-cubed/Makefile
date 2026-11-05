PORTNAME=	bsd-cubed
DISTVERSION=	0.1.0-beta3
CATEGORIES=	sysutils
MASTER_SITES=	https://github.com/denislemire/bsd-cubed/releases/download/v${DISTVERSION}/

MAINTAINER=	denis@lemire.name
COMMENT=	Experimental native BSD desktop session compatibility bridge
WWW=		https://github.com/denislemire/bsd-cubed

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

BUILD_DEPENDS=	dbus-run-session:devel/dbus
RUN_DEPENDS=	consolekit2>=2.0.0_4:sysutils/consolekit2 \
		dbus-daemon:devel/dbus

USES=		gnome pkgconfig
USE_GNOME=	glib20
USE_RC_SUBR=	bsddd

MAKE_ARGS=	PREFIX=${PREFIX} LOCALBASE=${LOCALBASE}
TEST_TARGET=	test

post-install:
	${MKDIR} ${STAGEDIR}${PREFIX}/etc/dbus-1/system.d
	${INSTALL_DATA} ${FILESDIR}/org.bsd3.login1.conf \
		${STAGEDIR}${PREFIX}/etc/dbus-1/system.d/org.bsd3.login1.conf

.include <bsd.port.mk>
