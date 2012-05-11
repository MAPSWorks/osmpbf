#include "inode.h"

#include "primitiveblockinputadaptor.h"
#include "osmformat.pb.h"

namespace osmpbf {

// INode

	INode::INode() : m_Private(NULL) {}
	INode::INode(const INode & other) : m_Private(other.m_Private) {
		if (m_Private) m_Private->refInc();
	}

	INode::INode(AbstractNodeInputAdaptor * data) : m_Private(data) {
		if (m_Private) m_Private->refInc();
	}

	INode::~INode() {
		if (m_Private) m_Private->refDec();
	}

	INode & INode::operator=(const INode & other) {
		if (m_Private)
			m_Private->refDec();
		m_Private = other.m_Private;
		if (m_Private)
			m_Private->refInc();
		return *this;
	}

// INodeStream

	INodeStream::INodeStream(PrimitiveBlockInputAdaptor * controller) : INode(new NodeStreamInputAdaptor(controller)) {}
	INodeStream::INodeStream(const INodeStream & other): INode(other) {}

// NodeStreamInputAdaptor

	NodeStreamInputAdaptor::NodeStreamInputAdaptor(PrimitiveBlockInputAdaptor * controller) :
		AbstractNodeInputAdaptor(controller, controller->m_NodesGroup, 0),
		m_DenseGroup(controller->m_DenseNodesGroup),
		m_NodesSize(m_Group ? m_Group->nodes_size() : 0),
		m_DenseNodesSize(m_DenseGroup ? m_DenseGroup->dense().id_size() : 0),
		m_Id(0),
		m_Lat(0), m_Lon(0),
		m_WGS84Lat(0), m_WGS84Lon(0)
	{
		m_DenseIndex = m_Index - m_NodesSize;
	}

	void NodeStreamInputAdaptor::next() {
		m_Index++;

		if (isNull())
			return;

		m_DenseIndex = m_Index - m_NodesSize;
		if (m_DenseIndex < 0) {
			m_Id = m_Group->nodes(m_Index).id();
			m_Lat = m_Group->nodes(m_Index).lat();
			m_Lon = m_Group->nodes(m_Index).lon();
		}
		else if (m_DenseIndex > 0) {
			if (m_Controller->denseNodesUnpacked()) {
				m_Id = m_DenseGroup->dense().id(m_DenseIndex);
				m_Lat = m_DenseGroup->dense().lat(m_DenseIndex);
				m_Lon = m_DenseGroup->dense().lon(m_DenseIndex);
			}
			else {
				m_Id += m_DenseGroup->dense().id(m_DenseIndex);
				m_Lat += m_DenseGroup->dense().lat(m_DenseIndex);
				m_Lon += m_DenseGroup->dense().lon(m_DenseIndex);
			}
		}
		else {
			m_Id = m_DenseGroup->dense().id(0);
			m_Lat = m_DenseGroup->dense().lat(0);
			m_Lon = m_DenseGroup->dense().lon(0);
		}

		m_WGS84Lat = m_Controller->toWGS84Lati(m_Lat);
		m_WGS84Lon = m_Controller->toWGS84Loni(m_Lon);
	}

	void NodeStreamInputAdaptor::previous() {
		m_Index--;

		if (isNull())
			return;

		m_DenseIndex = m_Index - m_NodesSize;
		if (m_DenseIndex < 0) {
			m_Id = m_Group->nodes(m_Index).id();
			m_Lat = m_Group->nodes(m_Index).lat();
			m_Lon = m_Group->nodes(m_Index).lon();
		}
		else if (m_DenseIndex > 0) {
			if (m_Controller->denseNodesUnpacked()) {
				m_Id = m_DenseGroup->dense().id(m_DenseIndex);
				m_Lat = m_DenseGroup->dense().lat(m_DenseIndex);
				m_Lon = m_DenseGroup->dense().lon(m_DenseIndex);
			}
			else {
				m_Id -= m_DenseGroup->dense().id(m_DenseIndex + 1);
				m_Lat -= m_DenseGroup->dense().lat(m_DenseIndex + 1);
				m_Lon -= m_DenseGroup->dense().lon(m_DenseIndex + 1);
			}
		}
		else {
			m_Id = m_DenseGroup->dense().id(0);
			m_Lat = m_DenseGroup->dense().lat(0);
			m_Lon = m_DenseGroup->dense().lon(0);
		}

		m_WGS84Lat = m_Controller->toWGS84Lati(m_Lat);
		m_WGS84Lon = m_Controller->toWGS84Loni(m_Lon);
	}

	int NodeStreamInputAdaptor::keysSize() const {
		return (m_DenseIndex < 0) ?
			m_Group->nodes(m_Index).keys_size() :
			m_Controller->queryDenseNodeKeyValIndex(m_DenseIndex * 2 + 1);
	}

	int NodeStreamInputAdaptor::keyId(int index) const {
		if (index < 0 || index > keysSize())
			return 0;

		return (m_DenseIndex < 0) ?
			m_Group->nodes(m_Index).keys(index) :
			m_DenseGroup->dense().keys_vals(m_Controller->queryDenseNodeKeyValIndex(m_DenseIndex * 2) + index * 2);
	}

	int NodeStreamInputAdaptor::valueId(int index) const {
		if (index < 0 || index > keysSize())
			return 0;

		return (m_DenseIndex < 0) ?
			m_Group->nodes(m_Index).vals(index) :
			m_DenseGroup->dense().keys_vals(m_Controller->queryDenseNodeKeyValIndex(m_DenseIndex * 2) + index * 2 + 1);
	}

	const std::string & NodeStreamInputAdaptor::key(int index) const {
		return m_Controller->queryStringTable(keyId(index));
	}

	const std::string & NodeStreamInputAdaptor::value(int index) const {
		return m_Controller->queryStringTable(valueId(index));
	}

// PlainNodeInputAdaptor

	PlainNodeInputAdaptor::PlainNodeInputAdaptor() : AbstractNodeInputAdaptor() {}
	PlainNodeInputAdaptor::PlainNodeInputAdaptor(PrimitiveBlockInputAdaptor * controller, PrimitiveGroup * group, int position) :
		AbstractNodeInputAdaptor(controller, group, position) {}

	int64_t PlainNodeInputAdaptor::id() {
		return m_Group->nodes(m_Index).id();
	}

	int64_t PlainNodeInputAdaptor::lati() {
		return m_Controller->toWGS84Lati(m_Group->nodes(m_Index).lat());
	}

	int64_t PlainNodeInputAdaptor::loni() {
		return m_Controller->toWGS84Loni(m_Group->nodes(m_Index).lon());
	}

	double PlainNodeInputAdaptor::latd() {
		return m_Controller->toWGS84Latd(m_Group->nodes(m_Index).lat());
	}

	double PlainNodeInputAdaptor::lond() {
		return m_Controller->toWGS84Lond(m_Group->nodes(m_Index).lon());
	}

	int64_t PlainNodeInputAdaptor::rawLat() const {
		return m_Group->nodes(m_Index).lat();
	}

	int64_t PlainNodeInputAdaptor::rawLon() const {
		return m_Group->nodes(m_Index).lon();
	}

	int PlainNodeInputAdaptor::keysSize() const {
		return m_Group->nodes(m_Index).keys_size();
	}

	int PlainNodeInputAdaptor::keyId(int index) const {
		return m_Group->nodes(m_Index).keys(index);
	}

	int PlainNodeInputAdaptor::valueId(int index) const {
		return m_Group->nodes(m_Index).vals(index);
	}

	const std::string & PlainNodeInputAdaptor::key(int index) const {
		return m_Controller->queryStringTable(m_Group->nodes(m_Index).keys(index));
	}

	const std::string & PlainNodeInputAdaptor::value(int index) const {
		return m_Controller->queryStringTable(m_Group->nodes(m_Index).vals(index));
	}

// DenseNodeInputAdaptor

	DenseNodeInputAdaptor::DenseNodeInputAdaptor() : AbstractNodeInputAdaptor() {}
	DenseNodeInputAdaptor::DenseNodeInputAdaptor(PrimitiveBlockInputAdaptor * controller, PrimitiveGroup * group, int position) :
		AbstractNodeInputAdaptor(controller, group, position) {}

	int64_t DenseNodeInputAdaptor::id() {
		if (!m_HasCachedId) {
			m_CachedId = m_Group->dense().id(0);
			for (int i = 0; i < m_Index; i++)
				m_CachedId += m_Group->dense().id(i);
		}

		return m_CachedId;
	}

	int64_t DenseNodeInputAdaptor::lati() {
		if (m_Controller->denseNodesUnpacked())
			return m_Controller->toWGS84Lati(m_Group->dense().lat(m_Index));

		if (!m_HasCachedLat) {
			m_CachedLat = m_Group->dense().lat(0);
			for (int i = 0; i < m_Index; i++)
				m_CachedLat += m_Group->dense().lat(i);
		}

		return m_Controller->toWGS84Lati(m_CachedLat);
	}

	int64_t DenseNodeInputAdaptor::loni() {
		if (m_Controller->denseNodesUnpacked())
			return m_Controller->toWGS84Loni(m_Group->dense().lon(m_Index));

		if (!m_HasCachedLon) {
			m_CachedLon = m_Group->dense().lon(0);
			for (int i = 0; i < m_Index; i++)
				m_CachedLon += m_Group->dense().lon(i);
		}

		return m_Controller->toWGS84Loni(m_CachedLon);
	}

	double DenseNodeInputAdaptor::latd() {
		return lati() * .000000001;
	}

	double DenseNodeInputAdaptor::lond() {
		return loni() * .000000001;
	}

	int64_t DenseNodeInputAdaptor::rawLat() const {
		return m_Group->dense().lat(m_Index);
	}

	int64_t DenseNodeInputAdaptor::rawLon() const {
		return m_Group->dense().lon(m_Index);
	}

	int DenseNodeInputAdaptor::keysSize() const {
		return (!m_Group->dense().keys_vals_size()) ? 0 : m_Controller->queryDenseNodeKeyValIndex(m_Index * 2 + 1);
	}

	int DenseNodeInputAdaptor::keyId(int index) const {
		return (index < 0 || index >= keysSize()) ? 0 : m_Group->dense().keys_vals(m_Controller->queryDenseNodeKeyValIndex(m_Index * 2) + index * 2);
	}

	int DenseNodeInputAdaptor::valueId(int index) const {
		return (index < 0 || index >= keysSize()) ? 0 : m_Group->dense().keys_vals(m_Controller->queryDenseNodeKeyValIndex(m_Index * 2) + index * 2 + 1);
	}

	const std::string & DenseNodeInputAdaptor::key(int index) const {
		return m_Controller->queryStringTable(keyId(index));
	}

	const std::string & DenseNodeInputAdaptor::value(int index) const {
		return m_Controller->queryStringTable(valueId(index));
	}
}
