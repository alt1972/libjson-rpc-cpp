/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file debug.h
 * @date 15.02.2014
 * @author toolmmy
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef LOG_H_
#define LOG_H_

namespace jsonrpc
{
	typedef void (*LogFunction)(const char *format, ...);
	typedef bool (*LogEnabledFunction)();

	extern LogFunction debug_log;
	extern LogEnabledFunction debug_enabled;
}

#endif /* LOG_H_ */
