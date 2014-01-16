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

#ifndef OSMPBF_IRELATION_H
#define OSMPBF_IRELATION_H

#include <cstdint>
#include <string>

#include "common.h"
#include "abstractprimitiveinputadaptor.h"
#include "iprimitive.h"

namespace crosby {
	namespace binary {
		class Relation;
	}
}

namespace osmpbf {
	class MemberStreamInputAdaptor : public generics::RefCountObject {
	public:
		MemberStreamInputAdaptor();
		MemberStreamInputAdaptor(const crosby::binary::Relation * data);

		bool isNull() const { return !m_Data || m_Index >= m_MaxIndex || m_Index < 0; }

		int64_t id() const { return m_CachedId; }
		PrimitiveType type() const;
		uint32_t roleId() const;

		void next();
		void previous();

	protected:
		const crosby::binary::Relation * m_Data;

		int m_Index;
		const int m_MaxIndex;

		int64_t m_CachedId;
	};

	class IMemberStream : public generics::RCWrapper< MemberStreamInputAdaptor > {
		friend class RelationInputAdaptor;
	public:
		IMemberStream(const IMemberStream & other);

		IMemberStream & operator=(const IMemberStream & other) {
			RCWrapper<MemberStreamInputAdaptor>::operator=(other);
			m_Controller = other.m_Controller;
			return *this;
		}

		virtual bool isNull() const { return !m_Controller || RCWrapper< MemberStreamInputAdaptor >::isNull() || RCWrapper< MemberStreamInputAdaptor >::m_Private->isNull(); }

		inline int64_t id() const { return m_Private->id(); }

		inline PrimitiveType type() const { return m_Private->type(); }

		inline uint32_t roleId() const { return m_Private->roleId(); }

		const std::string & role() const;

		inline void next() const { m_Private->next(); }

	protected:
		IMemberStream();
		IMemberStream(PrimitiveBlockInputAdaptor * controller, const crosby::binary::Relation * data);

		PrimitiveBlockInputAdaptor * m_Controller;
	};

	class RelationInputAdaptor : public AbstractPrimitiveInputAdaptor {
	public:
		RelationInputAdaptor();
		RelationInputAdaptor(PrimitiveBlockInputAdaptor * controller, const crosby::binary::Relation * data);

		virtual bool isNull() const { return AbstractPrimitiveInputAdaptor::isNull() || !m_Data; }

		virtual int64_t id();

		virtual int tagsSize() const;

		virtual uint32_t keyId(int index) const;
		virtual uint32_t valueId(int index) const;

		virtual int membersSize() const;

		virtual IMemberStream getMemberStream() const { return IMemberStream(m_Controller, m_Data); }

	protected:
		const crosby::binary::Relation * m_Data;
	};

	class IRelation : public IPrimitive {
		friend class PrimitiveBlockInputAdaptor;
	public:
		IRelation(const IRelation & other);

		inline IRelation & operator=(const IRelation & other) { IPrimitive::operator=(other); return *this; }

		inline int membersSize() const { return dynamic_cast< RelationInputAdaptor * >(m_Private)->membersSize(); }

		inline IMemberStream getMemberStream() const { return dynamic_cast< RelationInputAdaptor * >(m_Private)->getMemberStream(); }

	protected:
		IRelation();
		IRelation(RelationInputAdaptor * data);
	};

	class RelationStreamInputAdaptor : public RelationInputAdaptor {
	public:
		RelationStreamInputAdaptor();
		RelationStreamInputAdaptor(PrimitiveBlockInputAdaptor * controller);

		virtual bool isNull() const;

		void next();
		void previous();

	private:
		int m_Index;
		const int m_MaxIndex;
	};

	class IRelationStream : public IRelation {
	public:
		IRelationStream(PrimitiveBlockInputAdaptor * controller);
		IRelationStream(const IRelationStream & other);

		IRelationStream operator=(const IRelationStream & other) { IRelation::operator=(other); return *this; }

		inline void next() { static_cast<RelationStreamInputAdaptor *>(m_Private)->next(); }
		inline void previous() { static_cast<RelationStreamInputAdaptor *>(m_Private)->previous(); }

	protected:
		IRelationStream();
		IRelationStream(RelationInputAdaptor * data);
	};
}

#endif // OSMPBF_IRELATION_H
