# $FreeBSD$

PORTNAME=	qt45
PORTVERSION=	4.5.3
PORTREVISION=	1
CATEGORIES=	x11-toolkits linux
MASTER_SITES=	http://archives.fedoraproject.org/pub/archive/fedora/linux/updates/10/i386/ \
		http://archives.fedoraproject.org/pub/archive/fedora/linux/updates/10/SRPMS/
DISTNAME=	qt-${PORTVERSION}-${RPMVERSION}

MAINTAINER=	emulation@FreeBSD.org
COMMENT=	RPM of QT4 (Linux Fedora 10)

USES=		linux:f10
USE_LINUX_RPM=	yes
USE_LINUX=	fontconfig libmng png

RPMVERSION=	9.fc10
USE_LDCONFIG=	yes
PLIST=		${PKGDIR}/pkg-plist.${LINUX_RPM_ARCH}

.include <bsd.port.mk>
