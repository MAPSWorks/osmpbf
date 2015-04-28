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

/**
  * TagFilters:
  *
  *
  *
  * You can create a DAG out of Filters.
  *
  */

namespace osmpbf
{

template<class OSMInputPrimitive>
inline int findTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId)
{
	if (!keyId || !valueId)
		return -1;

	for (int i = 0; i < primitive.tagsSize(); i++)
		if (primitive.keyId(i) == keyId && primitive.valueId(i) == valueId)
			return i;

	return -1;
}

template<class OSMInputPrimitive>
inline int findKey(const OSMInputPrimitive & primitive, uint32_t keyId)
{
	if (!keyId)
		return -1;

	for (int i = 0; i < primitive.tagsSize(); ++i)
		if (primitive.keyId(i) == keyId)
			return i;

	return -1;
}

template<class OSMInputPrimitive>
inline bool hasTag(const OSMInputPrimitive & primitive, uint32_t keyId, uint32_t valueId)
{
	return findTag<OSMInputPrimitive>(primitive, keyId, valueId) > -1;
}

template<class OSMInputPrimitive>
inline bool hasKey(const OSMInputPrimitive & primitive, uint32_t keyId)
{
	return findKey<OSMInputPrimitive>(primitive, keyId) > -1;
}


class AbstractTagFilter : public generics::RefCountObject
{
public:
	AbstractTagFilter() : generics::RefCountObject(), m_Invert(false) {}
	virtual ~AbstractTagFilter() {}

	///if you associate an pbi with an filter then you have to rebuild the cache everytime you change the contents of the pbi
	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) = 0;

	virtual bool rebuildCache() = 0;

	/// this function is deprecated and will soon be removed, use rebuildCache() instead
	inline GENERICS_MARK_FUNC_DEPRECATED bool buildIdCache() { return rebuildCache(); }

	inline bool matches(const IPrimitive & primitive)
	{
		return m_Invert ? !p_matches(primitive) : p_matches(primitive);
	}

	inline bool invert()
	{
		m_Invert = !m_Invert;
		return m_Invert;
	}

	inline void setInverted(bool value)
	{
		m_Invert = value;
	}
	
	AbstractTagFilter * copy() const;

protected:
	typedef std::unordered_map<const AbstractTagFilter*, AbstractTagFilter*> CopyMap;
	virtual bool p_matches(const IPrimitive & primitive) = 0;

	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const = 0;
	inline AbstractTagFilter * copy(AbstractTagFilter * other, AbstractTagFilter::CopyMap & copies) const {
		return other->copy(copies);
	}

	bool m_Invert;
};

class PrimitiveTypeFilter: public AbstractTagFilter
{
public:
	PrimitiveTypeFilter(PrimitiveTypeFlags primitiveTypes);
	virtual ~PrimitiveTypeFilter();

	void setFilteredTypes(PrimitiveTypeFlags primitiveTypes);
	inline int filteredTypes() { return m_filteredPrimitives; }

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

private:
	int m_filteredPrimitives;
	const osmpbf::PrimitiveBlockInputAdaptor * m_PBI;
};

class AbstractMultiTagFilter : public AbstractTagFilter
{
public:
	AbstractMultiTagFilter() : AbstractTagFilter() {}
	virtual ~AbstractMultiTagFilter();

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;

	inline AbstractTagFilter * addChild(AbstractTagFilter * child)
	{
		if (child)
		{
			m_Children.push_front(child);
			child->rcInc();
		}
		return child;
	}

	template<typename T_ABSTRACT_TAG_FILTER_ITERATOR>
	inline void addChildren(T_ABSTRACT_TAG_FILTER_ITERATOR begin, const T_ABSTRACT_TAG_FILTER_ITERATOR & end)
	{
		for (; begin != end; ++begin)
		{
			addChild(*begin);
		}
	}

protected:
	typedef std::forward_list<AbstractTagFilter *> FilterList;
	virtual AbstractTagFilter* copy(CopyMap& copies) const override = 0;
	FilterList m_Children;
};

class OrTagFilter : public AbstractMultiTagFilter
{
public:
	OrTagFilter() : AbstractMultiTagFilter() {}

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

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override
	{
		if (m_PBI != pbi) m_KeyIdIsDirty = true;
		m_PBI = pbi;
	}

	virtual bool rebuildCache() override;

	int matchingTag() const
	{
		return m_LatestMatch;
	}

	void setKey(const std::string & key);
	inline const std::string & key() const { return m_Key; }

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

	uint32_t findId(const std::string & str);

	std::string m_Key;

	uint32_t m_KeyId;
	bool m_KeyIdIsDirty;

	inline void checkKeyIdCache()
	{
		if (m_KeyIdIsDirty)
		{
			m_KeyId = findId(m_Key);
			m_KeyIdIsDirty = false;
		}
	}

	int m_LatestMatch;

	const osmpbf::PrimitiveBlockInputAdaptor * m_PBI;
};

class StringTagFilter : public KeyOnlyTagFilter
{
public:
	StringTagFilter(const std::string & key, const std::string & value);

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override
	{
		if (m_PBI != pbi)
		{
			m_KeyIdIsDirty = true;
			m_ValueIdIsDirty = true;
		}
		m_PBI = pbi;
	}
	virtual bool rebuildCache() override;

	void setValue(const std::string & value);
	inline const std::string & value() const { return m_Value; }

	/// this function is deprecated and will soon be removed, use value() instead
	inline GENERICS_MARK_FUNC_DEPRECATED const std::string & Value() const { return value(); }

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

	std::string m_Value;

	uint32_t m_ValueId;
	bool m_ValueIdIsDirty;

	inline void checkValueIdCache()
	{
		if (m_ValueIdIsDirty)
		{
			m_ValueId = findId(m_Value);
			m_ValueIdIsDirty = false;
		}
	}
};

class MultiStringTagFilter : public KeyOnlyTagFilter
{
public:
	typedef std::unordered_set<uint32_t> IdSet;
	typedef std::unordered_set<std::string> ValueSet;

	MultiStringTagFilter(const std::string & key);
	MultiStringTagFilter(const std::string & key, std::initializer_list<std::string> l);
	template<typename T_STRING_ITERATOR>
	MultiStringTagFilter(const std::string & key, const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);

	virtual bool rebuildCache() override;

	template<typename T_STRING_ITERATOR>
	void setValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);

	inline void setValues(const std::set<std::string> & values) { setValues(values.cbegin(), values.cend()); }
	inline void setValues(const std::unordered_set<std::string> & values) { setValues(values.cbegin(), values.cend()); }
	inline void setValues(std::initializer_list<std::string> l) { setValues(l.begin(), l.end()); }

	void addValue(const std::string & value);

	void clearValues();

	inline MultiStringTagFilter & operator<<(const std::string & value)
	{
		addValue(value);
		return *this;
	}

	inline MultiStringTagFilter & operator<<(const char * value)
	{
		addValue(value);
		return *this;
	}

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

	void updateValueIds();

	IdSet m_IdSet;
	ValueSet m_ValueSet;
};

class MultiKeyTagFilter : public AbstractTagFilter
{
public:
	typedef std::unordered_set<uint32_t> IdSet;
	typedef std::unordered_set<std::string> ValueSet;

	MultiKeyTagFilter(std::initializer_list<std::string> l);
	template<typename T_STRING_ITERATOR>
	MultiKeyTagFilter(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;

	template<typename T_STRING_ITERATOR>
	void addValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end);

	inline void addValues(std::initializer_list<std::string> l) { addValues(l.begin(), l.end()); }

	void addValue(const std::string & value);

	void clearValues();

	inline MultiKeyTagFilter & operator<<(const std::string & value)
	{
		addValue(value);
		return *this;
	}

	inline MultiKeyTagFilter & operator<<(const char * value)
	{
		addValue(value);
		return *this;
	}

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

	const PrimitiveBlockInputAdaptor * m_PBI;
	bool m_KeyIdIsDirty;
	IdSet m_IdSet;
	ValueSet m_ValueSet;
};


/** Check for a @key that matches boolean value @value. Evaluates to false if key is not available */
class BoolTagFilter : public MultiStringTagFilter
{
public:
	BoolTagFilter(const std::string & key, bool value);

	virtual bool rebuildCache() override;

	void setValue(bool value);
	inline bool value() const { return m_Value; }

private:
	void setValues(const std::set< std::string > & values);
	inline void addValue(const std::string & value) { MultiStringTagFilter::addValue(value); }
	inline void clearValues() { MultiStringTagFilter::clearValues(); }

	inline MultiStringTagFilter & operator<<(const std::string & value)
	{
		MultiStringTagFilter::operator<<(value);
		return *this;
	}

	inline MultiStringTagFilter & operator<<(const char * value)
	{
		MultiStringTagFilter::operator<<(value);
		return *this;
	}

protected:
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;
	bool m_Value;
};

class IntTagFilter : public KeyOnlyTagFilter
{
public:
	IntTagFilter(const std::string & key, int value);

	virtual void assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) override;
	virtual bool rebuildCache() override;

	void setValue(int value);
	inline int value() const { return m_Value; }

protected:
	virtual bool p_matches(const IPrimitive & primitive) override;
	virtual AbstractTagFilter * copy(AbstractTagFilter::CopyMap & copies) const override;

	bool findValueId();

	int m_Value;
	uint32_t m_ValueId;
	bool m_ValueIdIsDirty;

	inline void checkValueIdCache()
	{
		if (m_ValueIdIsDirty)
			findValueId();
	}
};

/** @return needs to deleted after use, @param a,b return value will manage pointer */
inline AndTagFilter * newAnd(AbstractTagFilter * a, AbstractTagFilter * b)
{
	AndTagFilter * result = new AndTagFilter();
	result->addChild(a);
	result->addChild(b);
	return result;
}

/** @return needs to deleted after use, @param a,b return value will manage pointer */
inline OrTagFilter * newOr(AbstractTagFilter * a, AbstractTagFilter * b)
{
	OrTagFilter * result = new OrTagFilter();
	result->addChild(a);
	result->addChild(b);
	return result;
}

//definitions

template<typename T_STRING_ITERATOR>
MultiStringTagFilter::MultiStringTagFilter(const std::string & key, const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end) :
	KeyOnlyTagFilter(key),
	m_ValueSet(begin, end)
{
	updateValueIds();
}


template<typename T_STRING_ITERATOR>
void MultiStringTagFilter::setValues(const T_STRING_ITERATOR & begin, const T_STRING_ITERATOR & end)
{
	m_ValueSet.clear();
	m_ValueSet.insert(begin, end);
	updateValueIds();
}

template<typename T_STRING_ITERATOR>
MultiKeyTagFilter::MultiKeyTagFilter(const T_STRING_ITERATOR& begin, const T_STRING_ITERATOR& end) :
m_PBI(0),
m_KeyIdIsDirty(true),
m_ValueSet(begin, end)
{}

template<typename T_STRING_ITERATOR>
void MultiKeyTagFilter::addValues(const T_STRING_ITERATOR& begin, const T_STRING_ITERATOR& end)
{
	m_ValueSet.insert(begin, end);
	m_KeyIdIsDirty = true;
}
} // namespace osmpbf

#endif // OSMPBF_FILTER_H
