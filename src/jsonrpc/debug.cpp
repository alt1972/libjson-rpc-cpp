/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file debug.cpp
 * @date 15.02.2014
 * @author toolmmy
 * @license See attached LICENSE.txt
 ************************************************************************/

#include <stdarg.h>
#include <cstdio>
#include <stdlib.h>

#include "debug.h"

namespace jsonrpc
{
    static bool _debug_enabled()
    {
        static int debug_env = getenv("JSONRPC_VERBOSE") ? 1 : 0;
        return (debug_env == 1);
    }

	static void _debug_log_default(const char *format, ...)
	{



	  if (jsonrpc::debug_enabled())
	  {
		va_list args;
		va_start(args, format);

		fprintf(stderr, "jsonrpc: ");
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");

		va_end(args);
	  }
	}
}

jsonrpc::LogFunction jsonrpc::debug_log = _debug_log_default;
jsonrpc::LogEnabledFunction jsonrpc::debug_enabled = _debug_enabled;
