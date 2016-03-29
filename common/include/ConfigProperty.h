#ifndef __CONFIG_PROPERTY__H
#define __CONFIG_PROPERTY__H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>

#define CP_NAME_MAX	256
#define CP_VALUE_MAX	1024

class ConfigProperty {
public:
	ConfigProperty() {}
	ConfigProperty(const char *file);
	~ConfigProperty() {}
private:
	ConfigProperty(const ConfigProperty&);
	ConfigProperty& operator=(const ConfigProperty&);
public:
	int parse(const char *file);
public:
	const char *getString(const char *name, const char *def) const;
	const char *getString(const char *prefix, const char *child, const char *name, const char *def) const;

	int getInt(const char *name, int def) const;
	int getInt(const char *prefix, const char *child, const char *name, int def) const;
	
	double getDouble(const char *name, double def) const;
	double getDouble(const char *prefix, const char *child, const char *name, double def) const;

	// not a good interface? how?
	std::set<std::string> getChildren(const char *name) const;
private:
	typedef std::map<std::string, std::string> property_t;
	typedef property_t::iterator iterator;
	typedef property_t::const_iterator const_iterator;
	property_t properties;
};

#endif /* __CONFIG_PROPERTY__H */

