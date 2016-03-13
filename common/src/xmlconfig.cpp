#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <xmlconfig.h>

#include <sstream>

xmlconfig::icontext xmlconfig::end;

static xmlNodePtr xml_locate_child(xmlNodePtr node, const char *name, int index = 0)
{
	int cur = -1;
	for (xmlNodePtr iter = node->children;
		iter != NULL;
			iter = iter->next) {
		if (strcmp((const char *)iter->name, name) == 0) {
			if (++cur == index)
				return iter;
		}
	}

	return NULL;
}

static xmlNodePtr xml_locate_xpath(xmlNodePtr node, const char *path, std::string& last_name)
{
	const char *last_start = path, *last_end = path;

	while (*path) {
		const char *end = path;
		while (*end && !(*end == '/' || *end == '['))
			++end;
		std::string dir(last_start = path, last_end = end);
		if (*end == '/') {
			if (!dir.empty() && (node = xml_locate_child(node, dir.c_str())) == NULL)
				return NULL;
			path = end + 1;
		}
		else if (*end == '[') {
			int index = strtol(end + 1, (char **)&path, 0);
			if (!dir.empty() && (node = xml_locate_child(node, dir.c_str(), index)) == NULL)
				return NULL;
			if (*path != ']')
				return NULL;	// not followed by "]" 
			if (*++path == '/')
				++path;		// skip "/"
			else if (*path != '\0')
				return NULL;	// invalid char after "]"
		}
		else {
			node = xml_locate_child(node, dir.c_str());
			break;
		}
	}

	last_name.assign(last_start, last_end);
	return node;
}

static const char *xml_get_attr(xmlNodePtr node, const char *name)
{
	if (node == NULL)
		return NULL;

	for (xmlAttrPtr attr = node->properties;
		attr != NULL;
			attr = attr->next) {
		if (strcmp((const char *)attr->name, name) == 0) {
			return attr->children == NULL ? NULL
					: (char *)attr->children->content;
		}
	}

	return NULL;
}

static bool xml_set_attr(xmlNodePtr node, const char *name, const char *value)
{
	if (node == NULL)
	{
		return false;
	}
	return xmlSetProp(node, (const xmlChar*)name, (const xmlChar*)value);
}

__xmlcontext::__xmlcontext()
	: xml_node_(NULL), name_()
{}

__xmlcontext::__xmlcontext(xmlNodePtr node, const std::string& name)
	: xml_node_(node), name_(name)
{}

__xmlcontext __xmlcontext::get_context(const char *path) const
{
	std::string name;
	xmlNodePtr node = xml_locate_xpath(xml_node_, path, name);
	return __xmlcontext(node, name);
}

__xmlcontext __xmlcontext::set_context(const char *name) 
{
	xmlNodePtr childNode = xmlNewNode(NULL, BAD_CAST name);
	xmlAddChild(xml_node_, childNode);
	return __xmlcontext(childNode, name);
}


__xmlcontext __xmlcontext::operator++(int)
{
	__xmlcontext __old = *this;
	go_next(NULL);
	return __old;
}

__xmlcontext& __xmlcontext::operator++()
{
	go_next(NULL);
	return *this;
}

bool __xmlcontext::get_bool(const char *name, bool def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	if (isdigit(*val))
		return strtol(val, NULL, 0) != 0;
	if (*val == 't' || *val == 'T' || *val == 'y' || *val == 'Y')
		return true;
	return false;
}

char __xmlcontext::get_char(const char *name, char def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	return *val;
}

int __xmlcontext::get_int(const char *name, int def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	long d = strtol(val, NULL, 0);
	return d;
}

long __xmlcontext::get_long(const char *name, long def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	long d = strtol(val, NULL, 0);
	return d;
}

long long __xmlcontext::get_llong(const char *name, long long def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	long long d = strtoll(val, NULL, 0);
	return d;
}

double __xmlcontext::get_double(const char *name, double def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def;
	double d = strtod(val, NULL);
	return d;
}

std::string __xmlcontext::get_string(const char *name, const char *def) const
{
	const char *val = get_attr(name);
	if (val == NULL)
		return def == NULL ? std::string() : std::string(def);
	return std::string(val);
}

bool __xmlcontext::set_bool(const char *name, bool value)
{
	return set_attr(name, value ? "yes" : "no");
}

bool __xmlcontext::set_char(const char *name, char value)
{
	std::string str = &value;
	return set_attr(name, str.c_str());	
}

bool __xmlcontext::set_int(const char *name, int value)
{
	std::stringstream ss;
	ss << value;
	std::string str = ss.str();
	return set_attr(name, str.c_str());
}

bool __xmlcontext::set_long(const char *name, long value)
{
	std::stringstream ss;
	ss << value;
	std::string str = ss.str();
	return set_attr(name, str.c_str());
}

bool __xmlcontext::set_llong(const char *name, long long value)
{
	std::stringstream ss;
	ss << value;
	std::string str = ss.str();
	return set_attr(name, str.c_str());
}

bool __xmlcontext::set_double(const char *name, double value)
{
	std::stringstream ss;
	ss << value;
	std::string str = ss.str();
	return set_attr(name, str.c_str());
}

bool __xmlcontext::set_fixprecisiondouble(const char *name, double value)
{
	 std::ostringstream oss;
 	oss.precision(5);
 	oss.setf(std::ios::fixed ,std::ios::floatfield);
	oss << value;
	std::string str = oss.str();
	return set_attr(name, str.c_str());
}

bool __xmlcontext::set_string(const char *name, const std::string& value)
{
	return set_attr(name, value.c_str());
}

bool __xmlcontext::get_bool(const char *name, std::vector<bool>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		bool t = false;
		if (isdigit(*iter))
			t = strtol(iter, NULL, 0);
		else if (*iter == 't' || *iter == 'T' || *iter == 'y' || *iter == 'Y')
			t = true;
		result.push_back(t);
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

bool __xmlcontext::get_char(const char *name, std::vector<char>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		result.push_back(*iter);
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

bool __xmlcontext::get_int(const char *name, std::vector<int>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		result.push_back(strtol(iter, NULL, 0));
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

bool __xmlcontext::get_long(const char *name, std::vector<long>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		result.push_back(strtol(iter, NULL, 0));
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

bool __xmlcontext::get_llong(const char *name, std::vector<long long>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		result.push_back(strtoll(iter, NULL, 0));
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

bool __xmlcontext::get_double(const char *name, std::vector<double>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		next = strchr(iter, ',');
		result.push_back(strtod(iter, NULL));
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}

std::string __xmlcontext::get_Content()
{
	char *p = (char*)xmlNodeGetContent(xml_node_);
	if (p == NULL)
	{
		return "";
	}

	std::string str(p);
	free_value(p);
	
	return str;
}

bool __xmlcontext::set_Content(const char* value)
{
	xmlNodeSetContent(xml_node_,(const xmlChar*)value);
	return true;
}

bool __xmlcontext::free_value(char* value)
{
	if (value != NULL)xmlFree(value);
	return true;
}

bool __xmlcontext::isNullNode()
{
	if(xml_node_ == NULL)
	{
		return true;
	}
	return false;
}

bool __xmlcontext::get_string(const char *name, std::vector<std::string>& result) const
{
	const char *val = get_attr(name);
	const char *iter = val, *next;
	
	while (iter != NULL) {
		// TODO: more 
		next = strchr(iter, ',');
		const char *endptr = (next == NULL) ? iter + strlen(iter) : next;
		result.push_back(std::string(iter, endptr - iter));
		iter = (next == NULL) ? NULL : next + 1;
	}

	return true;
}
bool __xmlcontext::get_config_map(std::map<std::string, std::string>& result) const
{
	if (xml_node_ == NULL)
		return false;
	for (xmlAttrPtr attr = xml_node_->properties;
		attr != NULL;
			attr = attr->next) {
		result[(const char *)attr->name] = (const char *)attr->children->content;
	}

	return true;
}

bool __xmlcontext::equal(const __xmlcontext& other) const
{
	return xml_node_ == other.xml_node_;
}

void __xmlcontext::go_next(const char *name)
{
	if (xml_node_ == NULL)
		return;
	if (name == NULL)
		name = name_.c_str();
	for (xml_node_ = xml_node_->next;
		xml_node_ != NULL;
			xml_node_ = xml_node_->next) {
		if (strcmp((const char *)xml_node_->name, name) == 0)
			return;
	}

	return;
}

const char* __xmlcontext::get_attr(const char *name) const
{
	return xml_get_attr(xml_node_, name);
}

bool __xmlcontext::set_attr(const char *name, const char *value)
{
	return xml_set_attr(xml_node_, name, value);
}

xmlconfig::xmlconfig() : xml_doc_(NULL), xml_root_(NULL)
{}

xmlconfig::~xmlconfig()
{
	if (xml_doc_ != NULL)
		xmlFreeDoc(xml_doc_);
}

xmlconfig::xmlconfig(const char *buffer, int size)
{
	if (parse(buffer, size) < 0)
		throw std::runtime_error("parse(buffer, size) failed.");
	return;
}

xmlconfig::xmlconfig(const std::string& file)
{
	if (parse(file) < 0)
		throw std::runtime_error("parse(" + file + ") failed.");
	return;
}

int xmlconfig::save(const std::string& file)
{
	return xmlSaveFormatFileEnc(file.c_str(), xml_doc_, "UTF-8", 1);
}

int xmlconfig::createDoc()
{
	xml_doc_ =  xmlNewDoc(BAD_CAST "1.0");
	xml_root_ = xmlNewNode(NULL, BAD_CAST "res");
	xmlDocSetRootElement(xml_doc_, xml_root_);
	return 0;
}
int xmlconfig::createDoc(const char* tag)
{
	xml_doc_ =  xmlNewDoc(BAD_CAST "1.0");
	xml_root_ = xmlNewNode(NULL, BAD_CAST tag);
	xmlDocSetRootElement(xml_doc_, xml_root_);
	return 0;
}

__xmlcontext  xmlconfig::set_context(const char *name)
{
	xmlNodePtr childNode = xmlNewNode(NULL, BAD_CAST name);
	xmlAddChild(xml_root_, childNode);
	return __xmlcontext(childNode, name);
}

int xmlconfig::parse(const char *buffer, int size)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	
	if ((doc = xmlParseMemory(buffer, size)) == NULL) {
		return -1;
	}

	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		xmlFreeDoc(doc);
		return -2;
	}

	xml_doc_ = doc;
	xml_root_ = root;
	
	return 0;
}

int xmlconfig::parse(const std::string& file)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	
	if ((doc = xmlParseFile(file.c_str())) == NULL) {
		return -1;
	}

	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		xmlFreeDoc(doc);
		return -2;
	}

	xml_doc_ = doc;
	xml_root_ = root;
	
	return 0;
}

__xmlcontext xmlconfig::get_context(const char *path) const
{
	if (path == NULL || strlen(path) < 1)
		return __xmlcontext(xml_root_, "");
	if (*path == '/')
		++path;
	std::string name;
	return __xmlcontext(xml_locate_xpath(xml_root_, path, name), name);
}

bool xmlconfig::get_bool(const char *path, bool def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_bool(name, def);
}

char xmlconfig::get_char(const char *path, char def) const
{
	const char  *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_char(name, def);
}

int xmlconfig::get_int(const char *path, int def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_int(name, def);
}

long xmlconfig::get_long(const char *path, long def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_long(name, def);
}

long long xmlconfig::get_llong(const char *path, long long def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_llong(name, def);
}

double xmlconfig::get_double(const char *path, double def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_double(name, def);
}

std::string xmlconfig::get_string(const char *path, const char *def) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_string(name, def);
}

bool xmlconfig::get_bool(const char *path, std::vector<bool>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_bool(name, result);
}

bool xmlconfig::get_char(const char *path, std::vector<char>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_char(name, result);
}

bool xmlconfig::get_int(const char *path, std::vector<int>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_int(name, result);
}

bool xmlconfig::get_long(const char *path, std::vector<long>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_long(name, result);
}

bool xmlconfig::get_llong(const char *path, std::vector<long long>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_llong(name, result);
}

bool xmlconfig::get_double(const char *path, std::vector<double>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_double(name, result);
}

bool xmlconfig::get_string(const char *path, std::vector<std::string>& result) const
{
	const char *name;
	__xmlcontext ctx = get_context_and_last_name(path, name);
	return ctx.get_string(name, result);
}

bool xmlconfig::get_config_map(const char *path, std::map<std::string, std::string>& result) const
{
	__xmlcontext ctx = get_context(path);
	return ctx.get_config_map(result);
}

__xmlcontext xmlconfig::get_context_and_last_name(const char *path, const char *&name) const
{
	name = strrchr(path, '/');

	if (name == NULL) {
		name = path;
		return get_context(NULL);
	}

	std::string dir(path, name++);
	return get_context(dir.c_str());
}

#if 0
#include <iostream>
int main(int argc, char **argv)
{
try {
	xmlconfig config(argv[1]);
	xmlconfig::icontext ctx = config.get_context("/listener-list");

	while (ctx != xmlconfig::end) {
		int id = ctx.get_int("id", 99);
		std::string name = ctx.get_string("name");
		std::string bind = ctx.get_string("bind");

		std::cout << "id=" << id << ", name="
			  << name << ", bind=" << bind
			  << std::endl;

		for (xmlconfig::icontext it2 = ctx.get_context("listener");
			it2 != xmlconfig::end;
				++it2)
		{
			std::string at = it2.get_string("at");
			bool async = it2.get_bool("async");
			std::string proto = it2.get_string("protocol");
			std::string svc = it2.get_string("service");
			std::string bind = it2.get_string("bind");

			std::cout << "\tat=" << at << ", async="
				  << async << ", protocol=" << proto
				  << ", service=" << svc << ", bind="
				  << bind << std::endl;
		}

		++ctx;
	}
}
catch (std::exception& e) {
	std::cout << "catch a exception: " << e.what() << std::endl;
}

	return 0;
}
#endif
