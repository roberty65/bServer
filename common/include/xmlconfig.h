/* xmlconfig.h
 * Copyright by beyondy 2008-2010
 * All rights reserved.
 *
 *
**/
#ifndef __XMLCONFIG__H
#define __XMLCONFIG__H

#include <libxml/parser.h>
#include <string>
#include <vector>
#include <map>

class __xmlcontext {
public:
	__xmlcontext();
	__xmlcontext(xmlNodePtr node, const std::string& name);
public:
	//
	// interface
	//
	__xmlcontext get_context(const char *path) const;
	__xmlcontext set_context(const char *name);


	__xmlcontext operator++(int);
	__xmlcontext& operator++();

	bool get_bool(const char *name, bool def = false) const;
	char get_char(const char *name, char def = 0) const;
	int get_int(const char *name, int def = 0) const;
	long get_long(const char *name, long def = 0) const;
	long long get_llong(const char *name, long long def = 0) const;
	double get_double(const char *name, double def = 0.0) const;
	std::string get_string(const char *name, const char *def = NULL) const;

	bool set_bool(const char *name, bool value);
	bool set_char(const char *name, char value);
	bool set_int(const char* name, int value);
	bool set_long(const char* name, long value);
	bool set_llong(const char* name, long long value);
	bool set_double(const char* name, double value);
	bool set_fixprecisiondouble(const char *name, double value);
	bool set_string(const char* name, const std::string& value);

	bool get_bool(const char *name, std::vector<bool>& result) const;
	bool get_char(const char *name, std::vector<char>& result) const;
	bool get_int(const char *name, std::vector<int>& result) const;
	bool get_long(const char *name, std::vector<long>& result) const;
	bool get_llong(const char *name, std::vector<long long>& result) const;
	bool get_double(const char *name, std::vector<double>& result) const;
	bool get_string(const char *name, std::vector<std::string>& result) const;

	bool get_config_map(std::map<std::string, std::string>& result) const;
	bool equal(const __xmlcontext& other) const;
	std::string get_Content();
	bool set_Content(const char* value);
	bool free_value(char* value);
	bool isNullNode();
private:
	void go_next(const char *name);
	const char* get_attr(const char *name) const;
	bool set_attr(const char *name, const char *value);
private:
	xmlNodePtr xml_node_;
	std::string name_;
};

static inline bool operator==(const __xmlcontext& x, const __xmlcontext& y)
{
	return x.equal(y);
}

static inline bool operator!=(const __xmlcontext& x, const __xmlcontext& y)
{
	return !x.equal(y);
}

class xmlconfig {
public:
	typedef __xmlcontext icontext;
	static icontext end;
public:
	xmlconfig();
	xmlconfig(const char *buffer, int size);
	xmlconfig(const std::string& file);
	~xmlconfig();
public:
	//
	// config interface
	//
	int parse(const char *buffer, int size);
	int parse(const std::string& file);
	int save(const std::string& file);
	icontext get_context(const char *path) const;

	bool get_bool(const char *path, bool def = false) const;
	char get_char(const char *path, char def = 0) const;
	int get_int(const char *path, int def = 0) const;
	long get_long(const char *path, long def = 0) const;
	long long get_llong(const char *path, long long def = 0) const;
	double get_double(const char *name, double def = 0.0) const;
	std::string get_string(const char *path, const char *def = NULL) const;

	bool get_bool(const char *path, std::vector<bool>& result) const;
	bool get_char(const char *path, std::vector<char>& result) const;
	bool get_int(const char *path, std::vector<int>& result) const;
	bool get_long(const char *path, std::vector<long>& result) const;
	bool get_llong(const char *path, std::vector<long long>& result) const;
	bool get_double(const char *path, std::vector<double>& result) const;
	bool get_string(const char *path, std::vector<std::string>& result) const;

	bool get_config_map(const char *path, std::map<std::string, std::string>& result) const;
	xmlDocPtr getDocPtr(){return xml_doc_;}
	__xmlcontext  set_context(const char *name);
	int createDoc();
	int createDoc(const char* tag);

	xmlNodePtr getRoot(){return xml_root_;}

private:
	icontext get_context_and_last_name(const char *path, const char *&name) const;
private:
	xmlDocPtr xml_doc_;
	xmlNodePtr xml_root_;
};


#endif /*! __XMLCONFIG__H */

