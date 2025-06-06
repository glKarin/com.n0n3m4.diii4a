# $NetBSD: Makefile,v 1.22 2023/11/25 13:19:11 wiz Exp $

V= 1.10.6c
N= SeriousSamClassic

DISTNAME=	${N}-${V}
PKGNAME=	serioussam-${V}
PKGREVISION=	1
CATEGORIES=	games
MASTER_SITES=	${MASTER_SITE_GITHUB:=tx00100xt/}
GITHUB_PROJECT=	${N}
GITHUB_TAG=	${V}
EXTRACT_SUFX=	.tar.gz

MAINTAINER=	t.x00100x.t@yandex.ru
HOMEPAGE=	https://github.com/tx00100xt/SeriousSamClassic
COMMENT=	Open source version of game Serious Sam Classic
LICENSE=	gnu-gpl-v2

WRKSRC=		${WRKDIR}/${DISTNAME}

TOOL_DEPENDS+=	ImageMagick-[0-9]*:../../graphics/ImageMagick

USE_CMAKE=	yes
USE_TOOLS+=	bison flex
USE_LANGUAGES=	c gnu++11

USE_CXX_FEATURES+=	c++11

CMAKE_ARGS+=	-DUSE_ASM=ON
CMAKE_ARGS+=	-DUSE_SYSTEM_INSTALL=ON

.include "../../mk/bsd.prefs.mk"

.if ${OPSYS} == "NetBSD"
CFLAGS+=	-DIOAPI_NO_64
CXXFLAGS+=	-DIOAPI_NO_64
.endif

.if ${MACHINE_ARCH} == "i386"
CMAKE_ARGS+=	-DUSE_ASM=OFF
.endif

.if ${MACHINE_ARCH} == "armv7" || ${MACHINE_ARCH} == "aarch64"
CMAKE_ARGS+=	-DRPI4=ON
.endif

INSTALLATION_DIRS+=	man
INSTALLATION_DIRS+=	man/man1

post-install:
	${INSTALL_MAN} ${WRKSRC}/man/netbsd/serioussam.1 \
	    ${DESTDIR}${PREFIX}/man/man1
	${INSTALL_MAN} ${WRKSRC}/man/netbsd/serioussamclassic.1 \
	    ${DESTDIR}${PREFIX}/man/man1

.include "../../audio/libvorbis/buildlink3.mk"
.include "../../devel/SDL2/buildlink3.mk"
.include "../../devel/zlib/buildlink3.mk"
.include "../../sysutils/desktop-file-utils/desktopdb.mk"
.include "../../multimedia/libogg/buildlink3.mk"
.include "../../mk/bsd.pkg.mk"
