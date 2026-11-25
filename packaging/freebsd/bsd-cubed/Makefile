PORTNAME=	bsd-cubed
DISTVERSION=	0.2.0-alpha3
PORTREVISION=	8
CATEGORIES=	sysutils
MASTER_SITES=	https://github.com/denislemire/bsd-cubed/releases/download/v${DISTVERSION}/

MAINTAINER=	denis@lemire.name
COMMENT=	Experimental native BSD desktop session compatibility bridge
WWW=		https://github.com/denislemire/bsd-cubed

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

BUILD_DEPENDS=	dbus-run-session:devel/dbus
RUN_DEPENDS=	consolekit2>=2.0.0_5:sysutils/consolekit2 \
		dbus-daemon:devel/dbus \
		polkit>=0:sysutils/polkit

USES=		gnome pkgconfig
USE_GNOME=	glib20
USE_RC_SUBR=	bsddd

USERS=		_bsddd
GROUPS=		_bsddd
# Private overlay until the account is registered in the ports tree.
UID_FILES=	${FILESDIR}/UIDs
GID_FILES=	${FILESDIR}/GIDs
SUB_LIST+=	BSDDD_UID=${BSDDD_UID} BSDDD_GID=${BSDDD_GID}
BSDDD_UID!=	expr 829 + ${UID_OFFSET:U0}
BSDDD_GID!=	expr 829 + ${GID_OFFSET:U0}
SUB_FILES+=	pkg-pre-install
PKGPREINSTALL=	${WRKDIR}/pkg-pre-install

MAKE_ARGS=	PREFIX=${PREFIX} LOCALBASE=${LOCALBASE}
ALL_TARGET=	all
INSTALL_TARGET=	install
TEST_TARGET=	test

OPTIONS_DEFINE=	USERSESSION
OPTIONS_DEFAULT= USERSESSION
OPTIONS_SUB=	yes
USERSESSION_DESC= Experimental ordinary-user service graph API (not GNOME 49 desktop)
USERSESSION_RUN_DEPENDS= dinit>=0.19.4:sysutils/dinit
USERSESSION_BUILD_DEPENDS= dinit>=0.19.4:sysutils/dinit
USERSESSION_USES= python:3.11+,build
USERSESSION_MAKE_ARGS= PYTHON=${PYTHON_CMD}
USERSESSION_ALL_TARGET= user-session
USERSESSION_INSTALL_TARGET= install-user-session
USERSESSION_TEST_TARGET= test-user-session

pre-build:
.for registry in UIDs GIDs
	@${AWK} -F: '($$1 == "_bsddd" || $$3 == 829) && $$0 != expected { \
		print "Conflicting _bsddd assignment in " FILENAME; exit 1 }' \
		expected="`cat ${FILESDIR}/${registry}`" ${PORTSDIR}/${registry}
.endfor

post-install:
	${MKDIR} ${STAGEDIR}${PREFIX}/etc/dbus-1/system.d
	${INSTALL_DATA} ${FILESDIR}/org.bsd3.login1.conf \
		${STAGEDIR}${PREFIX}/etc/dbus-1/system.d/org.bsd3.login1.conf

.include <bsd.port.mk>
