/*
    This file is part of the osmpbf library.

    Copyright(c) 2012-2013 Oliver Groß.

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

#include "filter.h"

#include <cstdint>
#include <cstdlib>

#include "primitiveblockinputadaptor.h"
#include "iprimitive.h"
#include "irelation.h"

namespace osmpbf {

	// AbstractMultiTagFilter

	AbstractMultiTagFilter::~AbstractMultiTagFilter() {
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it)
			(*it)->rcDec();
	}

	void AbstractMultiTagFilter::assignInputAdaptor(const PrimitiveBlockInputAdaptor * pbi) {
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it)
			(*it)->assignInputAdaptor(pbi);
	}

	// OrTagFilter

	bool OrTagFilter::buildIdCache() {
		bool result = false;
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it)
			result = (*it)->buildIdCache() || result;

		return result;
	}

	bool OrTagFilter::p_matches(const IPrimitive & primitive) {
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it) {
			if ((*it)->matches(primitive))
				return true;
		}

		return false;
	}

	// AndTagFilter

	bool AndTagFilter::buildIdCache() {
		bool result = true;
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it)
			result = (*it)->buildIdCache() && result;

		return result;
	}

	bool AndTagFilter::p_matches(const IPrimitive & primitive) {
		for (FilterList::const_iterator it = m_Children.cbegin(); it != m_Children.cend(); ++it) {
			if (!(*it)->matches(primitive))
				return false;
		}

		return true;
	}

	// KeyOnlyTagFilter

	KeyOnlyTagFilter::KeyOnlyTagFilter(const std::string & key) :
		AbstractTagFilter(), m_Key(key), m_KeyId(0), m_KeyIdIsDirty(false), m_PBI(NULL) {}

	bool KeyOnlyTagFilter::buildIdCache() {
		m_KeyId = findId(m_Key);
		m_KeyIdIsDirty = false;

		if (!m_PBI) return true;
		if (m_PBI->isNull()) return false;

		return  m_KeyId;
	}

	void KeyOnlyTagFilter::setKey(const std::string & key) {
		m_Key = key;
		m_KeyIdIsDirty = true;
	}

	bool KeyOnlyTagFilter::p_matches(const IPrimitive & primitive) {
		if (m_Key.empty())
			return false;

		if (m_PBI) {
			if (m_PBI->isNull())
				return false;

			checkKeyIdCache();

			m_LatestMatch = findKey< IPrimitive >(primitive, m_KeyId);
			return m_LatestMatch > -1;
		}
		else {
			m_LatestMatch = -1;

			for (int i = 0; i < primitive.tagsSize(); ++i) {
				if ((primitive.key(i) == m_Key)) {
					m_LatestMatch = i;
					return true;
				}
			}

			return false;
		}
	}

	uint32_t KeyOnlyTagFilter::findId(const std::string & str) {
		if (!m_PBI || m_PBI->isNull())
			return 0;

		uint32_t id = 0;

		uint32_t stringTableSize = m_PBI->stringTableSize();

		for (id = 1; id < stringTableSize; ++id) {
			if (str == m_PBI->queryStringTable(id))
				break;
		}

		if (id >= stringTableSize)
			id = 0;

		return id;
	}

	// StringTagFilter

	StringTagFilter::StringTagFilter(const std::string & key, const std::string & value) :
		KeyOnlyTagFilter(key), m_Value(value), m_ValueId(0), m_ValueIdIsDirty(false) {}

	bool StringTagFilter::buildIdCache() {
		m_KeyId = findId(m_Key);
		m_KeyIdIsDirty = false;

		m_ValueId = findId(m_Value);
		m_ValueIdIsDirty = false;

		if (!m_PBI) return true;
		if (m_PBI->isNull()) return false;

		return m_KeyId && m_ValueId;
	}

	void StringTagFilter::setValue(const std::string & value) {
		m_Value = value;
		m_ValueIdIsDirty = true;
	}

	bool StringTagFilter::p_matches(const IPrimitive & primitive) {
		if (m_Key.empty())
			return false;

		if (m_PBI) {
			if (m_PBI->isNull())
				return false;

			checkKeyIdCache();
			checkValueIdCache();

			m_LatestMatch = findTag<IPrimitive>(primitive, m_KeyId, m_ValueId);
			return m_LatestMatch > -1;
		}

		m_LatestMatch = -1;

		for (int i = 0; i < primitive.tagsSize(); ++i) {
			if ((primitive.key(i) == m_Key) && primitive.value(i) == m_Value) {
				m_LatestMatch = i;
				return true;
			}
		}

		return false;
	}

	// MultiStringTagFilter

	MultiStringTagFilter::MultiStringTagFilter(const std::string & key) : KeyOnlyTagFilter(key) {}

	bool MultiStringTagFilter::buildIdCache() {
		m_KeyId = findId(m_Key);
		m_KeyIdIsDirty = false;

		updateValueIds();

		if (!m_PBI) return true;
		if (m_PBI->isNull()) return false;

		return m_KeyId && m_IdSet.size();
	}

	void MultiStringTagFilter::setValues(const std::set< std::string > & values) {
		m_ValueSet = values;
		updateValueIds();
	}

	void MultiStringTagFilter::addValue(const std::string & value) {
		m_ValueSet.insert(value);

		uint32_t valueId = findId(value);

		if (valueId)
			m_IdSet.insert(valueId);
	}

	bool MultiStringTagFilter::p_matches(const IPrimitive & primitive) {
		if (m_Key.empty())
			return false;

		m_LatestMatch = -1;

		if (m_PBI) {
			if (m_PBI->isNull())
				return false;

			checkKeyIdCache();

			if (!m_KeyId || m_IdSet.empty())
				return false;

			for (int i = 0; i < primitive.tagsSize(); i++) {
				if (primitive.keyId(i) == m_KeyId && m_IdSet.count(primitive.valueId(i))) {
					m_LatestMatch = -1;
					return true;
				}
			}

			return false;
		}
		else {
			for (int i = 0; i < primitive.tagsSize(); i++) {
				if (primitive.key(i) == m_Key && m_ValueSet.count(primitive.value(i))) {
					m_LatestMatch = -1;
					return true;
				}
			}

			return false;
		}
	}

	void MultiStringTagFilter::updateValueIds() {
		m_IdSet.clear();

		uint32_t valueId = 0;
		for (std::set< std::string >::const_iterator it = m_ValueSet.cbegin(); it != m_ValueSet.cend(); ++it) {
			valueId = findId(*it);

			if (valueId)
				m_IdSet.insert(valueId);
		}
	}

	// BoolTagFilter

	BoolTagFilter::BoolTagFilter(const std::string & key, bool value) :
		MultiStringTagFilter(key), m_Value(value)
	{
		if (m_Value) {
			addValue("true");
			addValue("yes");
			addValue("1");
		}
		else {
			addValue("false");
			addValue("no");
			addValue("0");
		}
	}

	void BoolTagFilter::setValue(bool value) {
		if (m_Value == value)
			return;

		m_Value = value;

		clearValues();

		if (m_Value) {
			addValue("true");
			addValue("yes");
			addValue("1");
		}
		else {
			addValue("false");
			addValue("no");
			addValue("0");
		}
	}

	// IntTagFilter

	IntTagFilter::IntTagFilter(const std::string & key, int value) :
		KeyOnlyTagFilter(key), m_Value(value) {}

	bool IntTagFilter::buildIdCache() {
		m_KeyId = findId(m_Key);
		m_KeyIdIsDirty = false;

		findValueId();

		if (!m_PBI) return true;
		if (m_PBI->isNull()) return false;

		return m_KeyId && m_ValueId;
	}

	void IntTagFilter::setValue(int value) {
		m_Value = value;
		findValueId();
	}

	bool IntTagFilter::findValueId() {
		m_ValueId = 0;
		m_ValueIdIsDirty = 0;

		if (!m_PBI)
			return true;

		uint32_t stringTableSize = m_PBI->stringTableSize();

		for (m_ValueId = 1; m_ValueId < stringTableSize; ++m_ValueId) {
			const std::string & tagValue = m_PBI->queryStringTable(m_ValueId);
			char * endptr;
			int intTagValue = strtol(tagValue.c_str(), &endptr, 10);

			if ((*endptr == '\0') && (intTagValue == m_Value))
				break;
		}

		if (m_ValueId >= stringTableSize)
			m_ValueId = 0;

		return m_ValueId;
	}

	bool IntTagFilter::p_matches(const IPrimitive & primitive) {
		if (m_Key.empty())
			return false;

		if (m_PBI) {
			if (m_PBI->isNull())
				return false;

			checkKeyIdCache();
			checkValueIdCache();

			m_LatestMatch = findTag<IPrimitive>(primitive, m_KeyId, m_ValueId);
			return m_LatestMatch > -1;
		}

		m_LatestMatch = -1;

		for (int i = 0; i < primitive.tagsSize(); ++i) {
			if (primitive.key(i) == m_Key) {
				char * endptr;
				int intTagValue = strtol(primitive.value(i).c_str(), &endptr, 10);

				if ((*endptr == '\0') && (intTagValue == m_Value)) {
					m_LatestMatch = i;
					return true;
				}
			}
		}

		return false;
	}

}
