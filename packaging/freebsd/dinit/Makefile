PORTNAME=	dinit
DISTVERSION=	0.19.4
DISTVERSIONPREFIX= v
CATEGORIES=	sysutils

MAINTAINER=	denis@lemire.name
COMMENT=	Service supervisor for explicit ordinary-user desktop sessions
WWW=		https://github.com/davmac314/dinit

LICENSE=	APACHE20
LICENSE_FILE=	${WRKSRC}/LICENSE

USES=		compiler:c++11-lang gmake
USE_GITHUB=	yes
GH_ACCOUNT=	davmac314

HAS_CONFIGURE=	yes
CONFIGURE_ARGS=	--prefix=${PREFIX} --exec-prefix=${PREFIX} \
		--sbindir=${PREFIX}/sbin --mandir=${PREFIX}/share/man \
		--disable-shutdown --disable-cgroups --disable-utmpx \
		--default-auto-restart=never
TEST_TARGET=	check
LDFLAGS+=	-lrt

# Deliberately no rc script, system service definitions or /sbin replacement.
PLIST_FILES=	sbin/dinit sbin/dinitctl sbin/dinitcheck sbin/dinit-monitor \
		share/man/man8/dinit.8.gz share/man/man8/dinitctl.8.gz \
		share/man/man8/dinitcheck.8.gz share/man/man8/dinit-monitor.8.gz \
		share/man/man5/dinit-service.5.gz

.include <bsd.port.mk>
