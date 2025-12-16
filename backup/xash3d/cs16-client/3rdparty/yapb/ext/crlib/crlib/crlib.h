//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <stdio.h>

#include <crlib/basic.h>
#include <crlib/memory.h>
#include <crlib/array.h>
#include <crlib/deque.h>
#include <crlib/binheap.h>
#include <crlib/files.h>
#include <crlib/lambda.h>
#include <crlib/http.h>
#include <crlib/library.h>
#include <crlib/hashmap.h>
#include <crlib/logger.h>
#include <crlib/cpuflags.h>
#include <crlib/mathlib.h>
#include <crlib/vector.h>
#include <crlib/random.h>
#include <crlib/ulz.h>
#include <crlib/color.h>
#include <crlib/detour.h>
#include <crlib/thread.h>
#include <crlib/timers.h>
#include <crlib/wavehelper.h>

CR_NAMESPACE_BEGIN

namespace types {
   using StringArray = Array <String>;
   using IntArray = Array <int>;
}

using namespace cr::types;

CR_NAMESPACE_END
