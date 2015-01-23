#pragma once

#include <intrinsics.h>

static inline void dbg_fatal(const char* file, int line)
{
	for (;;) __no_operation();
}

#define BUG()            do { dbg_fatal(__FILE__, __LINE__); } while(0)
#define BUG_ON(cond)     do { if (cond) dbg_fatal(__FILE__, __LINE__); } while(0)
