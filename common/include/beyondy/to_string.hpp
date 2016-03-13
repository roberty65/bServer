/* to_string.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
 * convert any type to a string, the type must be <<'able.
 * todo: support formater
 *
**/

#ifndef __BEYONDY__TO_STRING__HPP
#define __BEYONDY__TO_STRING__HPP

#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <string>
#include <iomanip>

namespace beyondy {

#ifndef TS_MAXSIZE
#define TS_MAXSIZE	1024
#endif /* TS_MAXSIZE */

template <typename _Tp>
static inline std::string toString(const _Tp& val)
{
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

static inline std::string toString(const std::string& val)
{
	return val;
}

static inline std::string toString(const char *format, ...)
{
	char sbuf[TS_MAXSIZE];
	va_list ap;
	int slen;

	va_start(ap, format);
	slen = vsnprintf(sbuf, sizeof(sbuf), format, ap);
	va_end(ap);

	sbuf[sizeof(sbuf) - 1] = '\0';
	return std::string(sbuf);
}

static inline std::string toString(const unsigned char* data, size_t size)
{
	std::ostringstream oss;

	for (size_t i = 0; i < size; ++i)
		oss << std::setw(2) << std::setfill('0') << std::hex
		    << (unsigned int)data[i] << (i == size - 1 ? "" : " ");
	return oss.str();
}

}; /* namespace beyondy */

#endif /*! __BEYONDY__TO_STRING__HPP */

