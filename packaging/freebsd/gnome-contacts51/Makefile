PORTNAME=	gnome-contacts
DISTVERSION=	51.0
CATEGORIES=	deskutils gnome
MASTER_SITES=	GNOME
DIST_SUBDIR=	gnome

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Contacts manager for gnome
WWW=		https://apps.gnome.org/Contacts/

LICENSE=	GPLv2
LICENSE_FILE=	${WRKSRC}/COPYING

BUILD_DEPENDS=	blueprint-compiler:devel/blueprint-compiler
LIB_DEPENDS=	libfolks.so:net-im/folks \
		libgee-0.8.so:devel/libgee \
		libglycin-gtk4-2.so:graphics/glycin \
		libgoa-1.0.so:net/gnome-online-accounts \
		libgstreamer-1.0.so:multimedia/gstreamer1 \
		libportal.so:deskutils/libportal \
		libportal-gtk4.so:deskutils/libportal-gtk4 \
		libqrencode.so:graphics/libqrencode

USES=		compiler:c11 desktop-file-utils gettext gnome localbase meson \
		pkgconfig tar:xz vala:build
USE_GNOME=	evolutiondataserver3 glib20 gtk40 libadwaita

GLIB_SCHEMAS=	org.gnome.Contacts.gschema.xml

OPTIONS_SUB=		yes
OPTIONS_DEFINE=		MANPAGES
OPTIONS_DEFAULT=	MANPAGES

MANPAGES_MESON_TRUE=	manpage
MANPAGES_USE=		GNOME=libxslt:build
MANPAGES_BUILD_DEPENDS=	docbook-xml>=0:textproc/docbook-xml \
			docbook-xsl-ns>=0:textproc/docbook-xsl-ns \
			docbook-xsl>=0:textproc/docbook-xsl

.include <bsd.port.mk>
