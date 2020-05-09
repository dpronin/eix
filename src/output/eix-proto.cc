// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include <config.h>  // IWYU pragma: keep

#ifdef WITH_PROTOBUF
#include "eixTk/diagnostics.h"
GCC_DIAG_OFF(sign-conversion)
#include "output/eix.pb.cc"  // NOLINT(build/include)
GCC_DIAG_ON(sign-conversion)
#endif
