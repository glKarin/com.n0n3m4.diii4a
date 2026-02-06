// Build-Config file for FTE's webless nearly-standard builds.
// these builds have crippled networking options to see if they fare better with malware false positives.
// to use: make FTE_CONFIG=fteqw_noweb

#include "config_fteqw.h"

#undef WEBCLIENT
#undef HAVE_HTTPSV
#undef TCPCONNECT
#undef FTPSERVER
#undef HAVE_TCP
#undef HAVE_GNUTLS
#undef HAVE_WINSSPI
#undef SUPPORT_ICE
#undef SUBSERVERS

