PORTNAME=	bsd-cubed
DISTVERSION=	0.1.0-beta2
CATEGORIES=	sysutils
MASTER_SITES=	https://github.com/denislemire/bsd-cubed/releases/download/v${DISTVERSION}/

MAINTAINER=	denis@lemire.name
COMMENT=	Experimental native BSD desktop session compatibility bridge
WWW=		https://github.com/denislemire/bsd-cubed

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

BUILD_DEPENDS=	dbus-run-session:devel/dbus
RUN_DEPENDS=	console-kit-daemon:sysutils/consolekit2

USES=		gnome pkgconfig
USE_GNOME=	glib20

MAKE_ARGS=	PREFIX=${PREFIX} LOCALBASE=${LOCALBASE}
TEST_TARGET=	test

.include <bsd.port.mk>
