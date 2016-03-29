/* ConfigProperty.cpp
  * Copyright by beyondy 2006 - 2020
  *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>

#include "ConfigProperty.h"

ConfigProperty::ConfigProperty(const char *file)
{
	if (parse(file) < 0) {
		char buf[512]; snprintf(buf, sizeof buf, "parse failed: %m"); buf[sizeof buf - 1] = 0;
		throw std::runtime_error(buf);
	}
}

int ConfigProperty::parse(const char *file)
{
	FILE *fp = fopen(file, "r");
	if (fp == NULL) return -1;
	
	int lineno = 0;
	char buf[(CP_NAME_MAX + CP_VALUE_MAX) * 2];

	while (fgets(buf, sizeof buf, fp) != NULL) {
		++lineno;
		if (*buf == '#') continue;

		int slen = strlen(buf);	
		while (slen > 0 && (buf[slen - 1] == '\n' || buf[slen - 1] == '\r'))
			--slen;
		buf[slen] = 0;

		char *endptr = buf + slen;
		if ((endptr = strchr(buf, '#')) != NULL) {
			*endptr = 0;
		}

#define SKIP_SPACE(p)	do { while (isspace(*(p))) (p)++; } while (0)
#define SKIP_CHARS(p)	do { while (*(p) && !isspace(*(p))) (p)++; } while (0)

		char *nptr = buf;
		SKIP_SPACE(nptr);
		if (*nptr == 0) continue; // empty-line

		char *vptr = strchr(nptr, '=');
		if (vptr == NULL) {
			fprintf(stderr, "No '=' found at line: %d\n", lineno);
			errno = EINVAL;
			fclose(fp);
			return -1;
		}
		else {
			*vptr++ = 0;
		}

		SKIP_SPACE(nptr); endptr = nptr; SKIP_CHARS(endptr); *endptr = 0;
		SKIP_SPACE(vptr); endptr = vptr; SKIP_CHARS(endptr); *endptr = 0;
#undef SKIP_SPACE
#undef SKIP_CHARS
		properties.insert(std::make_pair(std::string(nptr), std::string(vptr)));
	}

	fclose(fp);
	return 0;
}

const char *ConfigProperty::getString(const char *name, const char *def) const
{
	const_iterator iter = properties.find(name);
	if (iter == properties.end()) return def;
	return iter->second.c_str();
}

const char *ConfigProperty::getString(const char *prefix, const char *child, const char *name, const char *def) const
{
	char fullName[CP_NAME_MAX];
	snprintf(fullName, sizeof fullName, "%s.%s.%s", prefix, child, name); fullName[sizeof fullName - 1] = 0;
	return getString(fullName, def);
}

int ConfigProperty::getInt(const char *name, int def) const
{
	const char *val = getString(name, NULL);
	if (val == NULL || strlen(val) == 0) return def;

	char *eptr;
	int v = strtol(val, &eptr, 0);
	if (eptr == val) {
		// invalid number
		if (!strcasecmp(val, "true")  || !strcasecmp(val, "yes") || !strcasecmp(val, "t")  || !strcasecmp(val, "y"))
			return 1;
		if (!strcasecmp(val, "false") || !strcasecmp(val, "no") || !strcasecmp(val, "f") || !strcasecmp(val, "n"))
			return 0;
		return def;
	}

	if (*eptr == 'k' || *eptr == 'K') v *= 1024;
	else if (*eptr == 'm' || *eptr == 'M') v *= 1024 * 1024;
	else if (*eptr == 'g' || *eptr == 'G') v *= 1024 * 1024 * 1024;
	/* ignore others */

	return v;
}

int ConfigProperty::getInt(const char *prefix, const char *child, const char *name, int def) const
{
	char fullName[CP_NAME_MAX];
	snprintf(fullName, sizeof fullName, "%s.%s.%s", prefix, child, name); fullName[sizeof fullName - 1] = 0;
	return getInt(fullName, def);
}

double ConfigProperty::getDouble(const char *name, double def) const
{
	const char *val = getString(name, NULL);
	if (val == NULL || strlen(val) == 0) return def;

	return strtod(val, NULL);
}

double ConfigProperty::getDouble(const char *prefix, const char *child, const char *name, double def) const
{
	char fullName[CP_NAME_MAX];
	snprintf(fullName, sizeof fullName, "%s.%s.%s", prefix, child, name); fullName[sizeof fullName - 1] = 0;
	return getDouble(fullName, def);
}

std::set<std::string> ConfigProperty::getChildren(const char *name) const
{
	std::set<std::string> result;

	char prefix[CP_NAME_MAX];
	snprintf(prefix, sizeof prefix, "%s.", name);
	prefix[sizeof(prefix) - 1] = 0;

	int nlen = strlen(prefix);
	const_iterator first = properties.lower_bound(std::string(prefix));

	snprintf(prefix, sizeof prefix, "%s/", name);
	prefix[sizeof(prefix) - 1] = 0;
	const_iterator end = properties.upper_bound(std::string(prefix));

	for (const_iterator iter = first; iter != end; ++iter) {
		std::size_t dot = iter->first.find_first_of('.', nlen);
		if (dot != std::string::npos) {
			// after {name}. and next DOT
			result.insert(iter->first.substr(nlen, dot - nlen));
		}
	}

	return result;
}

