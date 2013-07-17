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

#ifndef OSMPBF_IWAY_H
#define OSMPBF_IWAY_H

#include <cstdint>
#include <string>

#include "abstractprimitiveinputadaptor.h"
#include "iprimitive.h"

#include <generics/fielditerator.h>

namespace crosby {
	namespace binary {
		class Way;
	}
}

namespace osmpbf {
	class PrimitiveBlockInputAdaptor;

	class WayInputAdaptor : public AbstractPrimitiveInputAdaptor {
	public:
		WayInputAdaptor();
		WayInputAdaptor(PrimitiveBlockInputAdaptor * controller, const crosby::binary::Way * data);

		virtual bool isNull() const { return AbstractPrimitiveInputAdaptor::isNull() || !m_Data; }

		virtual int64_t id();

		virtual int tagsSize() const;

		virtual uint32_t keyId(int index) const;
		virtual uint32_t valueId(int index) const;

		int refsSize() const;
		int64_t rawRef(int index) const;

		// warning: This methods complexity is O(n). It's here for convenience. You shouldn't
		//          call this method very often or with a high index parameter.
		int64_t ref(int index) const;

		generics::DeltaFieldConstForwardIterator<int64_t> refBegin() const;
		generics::DeltaFieldConstForwardIterator<int64_t> refEnd() const;

	protected:
		const crosby::binary::Way * m_Data;
	};

	class IWay : public IPrimitive {
	public:
		typedef generics::DeltaFieldConstForwardIterator<int64_t> RefIterator;
	private:
		friend class PrimitiveBlockInputAdaptor;
	public:
		IWay(const IWay & other);

		inline IWay & operator=(const IWay & other) { IPrimitive::operator=(other); return *this; }

		inline int64_t ref(int index) const { return dynamic_cast< WayInputAdaptor * >(m_Private)->ref(index); }
		inline int64_t rawRef(int index) const { return dynamic_cast< WayInputAdaptor * >(m_Private)->rawRef(index); }
		inline int refsSize() const { return dynamic_cast< WayInputAdaptor * >(m_Private)->refsSize(); }

		inline generics::DeltaFieldConstForwardIterator<int64_t> refBegin() const { return dynamic_cast< WayInputAdaptor * >(m_Private)->refBegin(); }
		inline generics::DeltaFieldConstForwardIterator<int64_t> refEnd() const { return dynamic_cast< WayInputAdaptor * >(m_Private)->refEnd(); }

	protected:
		IWay();
		IWay(WayInputAdaptor * data);
	};

	class WayStreamInputAdaptor : public WayInputAdaptor {
	public:
		WayStreamInputAdaptor();
		WayStreamInputAdaptor(PrimitiveBlockInputAdaptor * controller);

		virtual bool isNull() const;

		void next();
		void previous();

	private:
		int m_Index;
		const int m_MaxIndex;
	};

	class IWayStream : public IWay {
	public:
		IWayStream(PrimitiveBlockInputAdaptor * controller);
		IWayStream(const IWayStream & other);

		inline IWayStream & operator=(IWayStream & other) { IWay::operator=(other); return *this; }

		inline void next() { dynamic_cast<WayStreamInputAdaptor *>(m_Private)->next(); }
		inline void previous() { dynamic_cast<WayStreamInputAdaptor *>(m_Private)->previous(); }

	protected:
		IWayStream();
		IWayStream(WayInputAdaptor * data);
	};
}

#endif // OSMPBF_OSMWAY_H
