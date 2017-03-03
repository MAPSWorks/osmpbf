/*
    This file is part of the osmpbf library.

    Copyright(c) 2012-2014 Oliver Groß.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, see
    <http://www.gnu.org/licenses/>.
 */

#ifndef OSMPBF_FILTER_H
#define OSMPBF_FILTER_H

#include <osmpbf/common_input.h>

#include <generics/macros.h>
#include <generics/refcountobject.h>

#include <forward_list>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <regex>

/**
  * TagFilters:
  *
  *
  *
  * You can create a DAG out of Filters.
  * It is possible to speed up filtering by assigning a PrimitiveBlockInputAdaptor to a filter.
  * Note that the assigned adaptor needs to be valid during usage of the filter.
  * 
  * 
  *
  */

namespace osmpbf
{

class AndTagFilter;
class OrTagFilter;
class InversionFilter;
class ConstantReturnFilter;
class PrimitiveTypeFilter;
class BoolTagFilter;
class IntTagFilter;
class KeyOnlyTagFilter;
class KeyValueTagFilter;
class KeyMultiValueTagFilter;
class MultiKeyMultiValueTagFilter;
class RegexKeyTagFilter;

template<class OSMInputPrimitive>
int findTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId);

template<class OSMInputPrimitive>
int findKey(const OSMInputPrimitive & primitive, uint32_t keyId);

template<class OSMInputPrimitive>
bool hasTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId);

template<class OSMInputPrimitive>
bool hasKey(const OSMInputPrimitive & primitive, uint32_t keyId);

class AbstractTagFilter : public generics::RefCountObject
{
public:
	AbstractTagFilter();
	virtual ~AbstractTagFilter();
	AbstractTagFilter * copy() const;
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) = 0;
	virtual bool rebuildCache() = 0;
public:
	bool matches(const IPrimitive & primitive);
protected:
	typedef std::unordered_map<const AbstractTagFilter*, AbstractTagFilter*> CopyMap;
	virtual bool p_matches(const IPrimitive & primitive) = 0;
protected:
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const = 0;
	AbstractTagFilter * copy(AbstractTagFilter * other, AbstractTagFilter::CopyMap & copies) const;
};

class AbstractMultiTagFilter : public AbstractTagFilter
{
public:
	AbstractMultiTagFilter() : AbstractTagFilter() {}
	virtual ~AbstractMultiTagFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
public:
	AbstractTagFilter * addChild(AbstractTagFilter * child);
	template<typename T_ABSTRACT_TAG_FILTER_ITERATOR>
	void addChildren(T_ABSTRACT_TAG_FILTER_ITERATOR begin, const T_ABSTRACT_TAG_FILTER_ITERATOR & end);
protected:
	typedef std::forward_list<AbstractTagFilter *> FilterList;
	virtual AbstractTagFilter* copy(CopyMap& copies) const override = 0;
protected:
	FilterList m_Children;
};

typedef generics::RCPtr<AbstractTagFilter> RCFilterPtr;

class CopyFilterPtr {
private:
	void safe_bool_func() {}
	typedef void (CopyFilterPtr:: * safe_bool_type) ();
public:
	CopyFilterPtr();
	explicit CopyFilterPtr(const RCFilterPtr & other);
	CopyFilterPtr(const CopyFilterPtr & other);
	CopyFilterPtr(CopyFilterPtr && other);
	virtual ~CopyFilterPtr();
	CopyFilterPtr & operator=(const CopyFilterPtr & other);
	CopyFilterPtr & operator=(CopyFilterPtr && other);
	bool operator==(const CopyFilterPtr & other);
	bool operator!=(const CopyFilterPtr & other);
	AbstractTagFilter & operator*();
	const AbstractTagFilter & operator*() const;
	AbstractTagFilter * operator->();
	const AbstractTagFilter * operator->() const;
	AbstractTagFilter * get();
	const AbstractTagFilter * get() const;
	operator safe_bool_type() const;
	void reset(const RCFilterPtr & filter);
	void reset(RCFilterPtr && filter);
private:
	RCFilterPtr & priv();
	const RCFilterPtr priv() const;
private:
	RCFilterPtr m_Private;
};

///Inverts the result of another filter
class InversionFilter: public AbstractTagFilter {
public:
	InversionFilter();
	InversionFilter(AbstractTagFilter * child);
	InversionFilter(const InversionFilter & other) = delete;
	InversionFilter operator=(const InversionFilter & other) = delete;
	virtual ~InversionFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	void setChild(AbstractTagFilter * child);
	AbstractTagFilter * child();
	const AbstractTagFilter * child() const;
public:
	///invert a given filter (removes InversionFilter if applicable)
	static void invert(RCFilterPtr & filter);
	static RCFilterPtr invert(AbstractTagFilter * filter);
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
private:
	AbstractTagFilter * m_child;
};

//always returns either true or false
class ConstantReturnFilter: public AbstractTagFilter
{
public:
	ConstantReturnFilter(bool returnValue);
	virtual ~ConstantReturnFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	void setValue(bool returnValue);
	bool value() const;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	bool m_returnValue;
};

class PrimitiveTypeFilter: public AbstractTagFilter
{
public:
	PrimitiveTypeFilter(PrimitiveTypeFlags primitiveTypes);
	virtual ~PrimitiveTypeFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	void setFilteredTypes(PrimitiveTypeFlags primitiveTypes);
	int filteredTypes() const;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
private:
	int m_filteredPrimitives;
	const osmpbf::PrimitiveBlockInputAdaptor * m_PBI;
};

class OrTagFilter : public AbstractMultiTagFilter
{
public:
	OrTagFilter() : AbstractMultiTagFilter() {}
	OrTagFilter(std::initializer_list<AbstractTagFilter*> l);
public:
	virtual bool rebuildCache() override;
protected:
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
private:
	virtual bool p_matches(const IPrimitive & primitive) override;
};

class AndTagFilter : public AbstractMultiTagFilter
{
public:
	AndTagFilter() : AbstractMultiTagFilter() {}
	AndTagFilter(std::initializer_list<AbstractTagFilter*> l);
public:
	virtual bool rebuildCache() override;
protected:
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
private:
	virtual bool p_matches(const IPrimitive & primitive) override;
};

class KeyOnlyTagFilter : public AbstractTagFilter
{
public:
	KeyOnlyTagFilter(const std::string & key);
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	int matchingTag() const;
	void setKey(const std::string & key);
	const std::string & key() const;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	uint32_t findId(const std::string & str);
	void checkKeyIdCache();
protected:
	std::string m_Key;
	uint32_t m_KeyId;
	bool m_KeyIdIsDirty;
	int m_LatestMatch;
	const osmpbf::PrimitiveBlockInputAdaptor * m_PBI;
};

///A single key, single value tag filter
class KeyValueTagFilter : public KeyOnlyTagFilter
{
public:
	KeyValueTagFilter(const std::string & key, const std::string & value);
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	void setValue(const std::string & value);
	const std::string & value() const;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	void checkValueIdCache();
protected:
	std::string m_Value;
	uint32_t m_ValueId;
	bool m_ValueIdIsDirty;
};

///A single key, multiple values filter
class KeyMultiValueTagFilter : public KeyOnlyTagFilter
{
public:
	typedef std::unordered_set<uint32_t> IdSet;
	typedef std::unordered_set<std::string> ValueSet;
public:
	KeyMultiValueTagFilter(const std::string & key);
	KeyMultiValueTagFilter(const std::string & key, std::initializer_list<std::string> l);
	template<typename T_STRING_ITERATOR>
	KeyMultiValueTagFilter(const std::string & key, const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);
public:
	virtual bool rebuildCache() override;
public:
	template<typename T_STRING_ITERATOR>
	void setValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);
	void setValues(const std::set<std::string> & values);
	void setValues(const std::unordered_set<std::string> & values);
	void setValues(std::initializer_list<std::string> l);
	void addValue(const std::string & value);
	KeyMultiValueTagFilter & operator<<(const std::string & value);
	KeyMultiValueTagFilter & operator<<(const char * value);
	void clearValues();
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	void updateValueIds();
protected:
	IdSet m_IdSet;
	ValueSet m_ValueSet;
};

///A multi-key, any-value filter
class MultiKeyTagFilter : public AbstractTagFilter
{
public:
	typedef std::unordered_set<uint32_t> IdSet;
	typedef std::unordered_set<std::string> ValueSet;
public:
	MultiKeyTagFilter(std::initializer_list<std::string> l);
	template<typename T_STRING_ITERATOR>
	MultiKeyTagFilter(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	template<typename T_STRING_ITERATOR>
	void addValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);
	void addValues(std::initializer_list<std::string> l);
	void addValue(const std::string & value);
	MultiKeyTagFilter & operator<<(const std::string & value);
	MultiKeyTagFilter & operator<<(const char * value);
	void clearValues();
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	const PrimitiveBlockInputAdaptor * m_PBI;
	bool m_KeyIdIsDirty;
	IdSet m_IdSet;
	ValueSet m_KeySet;
};

///A multiple keys, multiple-values filter
class MultiKeyMultiValueTagFilter : public AbstractTagFilter
{
public:
	MultiKeyMultiValueTagFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	template<typename T_STRING_ITERATOR>
	void addValues(const std::string & key, const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);
	void clearValues();
protected:
	typedef std::unordered_map<std::string, std::unordered_set<std::string> > ValueMap;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	ValueMap m_ValueMap;
};

///A regex based key filter.
class RegexKeyTagFilter : public AbstractTagFilter
{
public:
	RegexKeyTagFilter();
	RegexKeyTagFilter(const std::string & regexString, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
	RegexKeyTagFilter(const std::regex & regex, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
	template<typename T_OCTET_ITERATOR>
	RegexKeyTagFilter(const T_OCTET_ITERATOR & begin, const T_OCTET_ITERATOR & end, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
	virtual ~RegexKeyTagFilter();
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	template<typename T_OCTET_ITERATOR>
	void setRegex(const T_OCTET_ITERATOR & begin, const T_OCTET_ITERATOR & end, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
	void setRegex(const std::regex & regex, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
	void setRegex(const std::string & regexString, std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	const PrimitiveBlockInputAdaptor * m_PBI;
	std::regex m_regex;
	std::regex_constants::match_flag_type m_matchFlags;
	std::unordered_set<int> m_IdSet;
	bool m_dirty;
private:
	RegexKeyTagFilter(std::regex_constants::match_flag_type flags);
};

/** Check for a @key that matches boolean value @value. Evaluates to false if key is not available */
class BoolTagFilter : public KeyMultiValueTagFilter
{
public:
	BoolTagFilter(const std::string & key, bool value);
public:
	virtual bool rebuildCache() override;
public:
	void setValue(bool value);
	bool value() const;
private:
	void setValues(const std::set< std::string > & values);
	void addValue(const std::string & value);
	void clearValues();
	KeyMultiValueTagFilter & operator<<(const std::string & value);
	KeyMultiValueTagFilter & operator<<(const char * value);
protected:
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
	bool m_Value;
};

class IntTagFilter : public KeyOnlyTagFilter
{
public:
	IntTagFilter(const std::string & key, int value);
public:
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;
public:
	void setValue(int value);
	int value() const;
protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
protected:
	bool findValueId();
	void checkValueIdCache();
protected:
	int m_Value;
	uint32_t m_ValueId;
	bool m_ValueIdIsDirty;
};

AndTagFilter * newAnd(AbstractTagFilter * a, AbstractTagFilter * b);
OrTagFilter * newOr(AbstractTagFilter * a, AbstractTagFilter * b);

//definitions

template<class OSMInputPrimitive>
int findTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId)
{
	if (!keyId || !valueId)
		return -1;

	for (int i = 0; i < primitive.tagsSize(); i++)
		if (primitive.keyId(i) == keyId && primitive.valueId(i) == valueId)
			return i;

	return -1;
}

template<class OSMInputPrimitive>
int findKey(const OSMInputPrimitive & primitive, uint32_t keyId)
{
	if (!keyId)
		return -1;

	for (int i = 0; i < primitive.tagsSize(); ++i)
		if (primitive.keyId(i) == keyId)
			return i;

	return -1;
}

template<class OSMInputPrimitive>
bool hasTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId)
{
	return findTag<OSMInputPrimitive>(primitive, keyId, valueId) > -1;
}

template<class OSMInputPrimitive>
bool hasKey(const OSMInputPrimitive & primitive, uint32_t keyId)
{
	return findKey<OSMInputPrimitive>(primitive, keyId) > -1;
}

template<typename T_ABSTRACT_TAG_FILTER_ITERATOR>
void AbstractMultiTagFilter::addChildren(T_ABSTRACT_TAG_FILTER_ITERATOR begin, const T_ABSTRACT_TAG_FILTER_ITERATOR & end)
{
	for (; begin != end; ++begin)
	{
		addChild(*begin);
	}
}

template<typename T_STRING_ITERATOR>
KeyMultiValueTagFilter::KeyMultiValueTagFilter(const std::string & key, const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end) :
KeyOnlyTagFilter(key),
m_ValueSet(begin, end)
{
	updateValueIds();
}


template<typename T_STRING_ITERATOR>
void KeyMultiValueTagFilter::setValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end)
{
	m_ValueSet.clear();
	m_ValueSet.insert(begin, end);
	updateValueIds();
}

template<typename T_STRING_ITERATOR>
MultiKeyTagFilter::MultiKeyTagFilter(const T_STRING_ITERATOR& begin, const T_STRING_ITERATOR& end) :
m_PBI(0),
m_KeyIdIsDirty(true),
m_KeySet(begin, end)
{}

template<typename T_STRING_ITERATOR>
void MultiKeyTagFilter::addValues(const T_STRING_ITERATOR& begin, const T_STRING_ITERATOR& end)
{
	m_KeySet.insert(begin, end);
	m_KeyIdIsDirty = true;
}

template<typename T_STRING_ITERATOR>
void MultiKeyMultiValueTagFilter::addValues(const std::string& key, const T_STRING_ITERATOR& begin, const T_STRING_ITERATOR& end)
{
	m_ValueMap[key].insert(begin, end);
}

template<typename T_OCTET_ITERATOR>
RegexKeyTagFilter::RegexKeyTagFilter(const T_OCTET_ITERATOR & begin, const T_OCTET_ITERATOR & end, std::regex_constants::match_flag_type flags) :
m_PBI(0),
m_regex(begin, end),
m_matchFlags(flags),
m_dirty(true)
{}

template<typename T_OCTET_ITERATOR>
void RegexKeyTagFilter::setRegex(const T_OCTET_ITERATOR & begin, const T_OCTET_ITERATOR & end, std::regex_constants::match_flag_type flags) {
	m_regex.assign(begin, end);
	m_matchFlags = flags;
	m_dirty = true;
}

} // namespace osmpbf

#endif // OSMPBF_FILTER_H
