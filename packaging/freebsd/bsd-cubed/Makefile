PORTNAME=	bsd-cubed
DISTVERSION=	0.1.0-beta3
CATEGORIES=	sysutils
MASTER_SITES=	https://packages.ehws.generic.business/sources/bsd-cubed/v${DISTVERSION}/ \
		https://github.com/denislemire/bsd-cubed/releases/download/v${DISTVERSION}/

MAINTAINER=	denis@lemire.name
COMMENT=	Experimental native BSD desktop session compatibility bridge
WWW=		https://github.com/denislemire/bsd-cubed

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

BUILD_DEPENDS=	dbus-run-session:devel/dbus
RUN_DEPENDS=	consolekit2>=2.0.0_5:sysutils/consolekit2 \
		dbus-daemon:devel/dbus

USES=		gnome pkgconfig
USE_GNOME=	glib20
USE_RC_SUBR=	bsddd

MAKE_ARGS=	PREFIX=${PREFIX} LOCALBASE=${LOCALBASE}
ALL_TARGET=	all
INSTALL_TARGET=	install
TEST_TARGET=	test

OPTIONS_DEFINE=	USERSESSION
OPTIONS_SUB=	yes
USERSESSION_DESC= Experimental ordinary-user service graph API (not GNOME 49 desktop)
USERSESSION_RUN_DEPENDS= dinit>=0.19.4:sysutils/dinit
USERSESSION_BUILD_DEPENDS= dinit>=0.19.4:sysutils/dinit
USERSESSION_USES= python:3.11+,build
USERSESSION_MAKE_ARGS= PYTHON=${PYTHON_CMD}
USERSESSION_ALL_TARGET= user-session
USERSESSION_INSTALL_TARGET= install-user-session
USERSESSION_TEST_TARGET= test-user-session

post-install:
	${MKDIR} ${STAGEDIR}${PREFIX}/etc/dbus-1/system.d
	${INSTALL_DATA} ${FILESDIR}/org.bsd3.login1.conf \
		${STAGEDIR}${PREFIX}/etc/dbus-1/system.d/org.bsd3.login1.conf

.include <bsd.port.mk>
