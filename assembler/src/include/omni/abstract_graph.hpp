#ifndef ABSTRACT_GRAPH_HPP_
#define ABSTRACT_GRAPH_HPP_

#include <vector>
#include <set>
#include <cstring>
#include "sequence/seq.hpp"
#include "sequence/sequence.hpp"
#include "logging.hpp"
#include "sequence/nucl.hpp"
#include "omni_utils.hpp"
#include "observable_graph.hpp"

namespace omnigraph {

template<typename VertexIdT, typename VertexData, typename EdgeIdT, typename EdgeData, class DataMaster, typename VertexIterator/* = typename set<VertexIdT>::iterator*/>
class AbstractGraph: public ObservableGraph<VertexIdT, EdgeIdT, VertexIterator> {
	typedef ObservableGraph<VertexIdT, EdgeIdT, VertexIterator> base;
	//todo maybe rename template params themselves???
public:
	typedef VertexIdT VertexId;
	typedef EdgeIdT EdgeId;
	typedef VertexData VertexDataType;
	typedef EdgeData EdgeDataType;
	typedef DataMaster DataMasterType;
private:
	//todo think of necessity to pull these typedefs through hierarchy
	DataMaster master_;

	virtual VertexId HiddenAddVertex(const VertexData &data) = 0;

	virtual void HiddenDeleteVertex(VertexId v) = 0;

	virtual EdgeId HiddenAddEdge(VertexId v1, VertexId v2,
			const EdgeData &data) = 0;

	virtual void HiddenDeleteEdge(EdgeId edge) = 0;

	virtual vector<EdgeId> CorrectMergePath(const vector<EdgeId>& path) = 0;

	virtual vector<EdgeId> EdgesToDelete(const vector<EdgeId> &path) = 0;

	virtual vector<VertexId> VerticesToDelete(const vector<EdgeId> &path) = 0;

	void DeleteAllOutgoing(VertexId v) {
		vector<EdgeId> out = OutgoingEdges(v);
		TRACE("DeleteAllOutgoing "<<out.size());
		for (auto it = out.begin(); it != out.end(); ++it) {
			TRACE("DeleteOutgoing "<<*it);
			DeleteEdge(*it);
			TRACE("DeleteOutgoing ok");
		}
	}

	void DeleteAllIncoming(VertexId v) {
		vector<EdgeId> in = IncomingEdges(v);
		TRACE("DeleteAllIncoming "<<in.size());
		for (auto it = in.begin(); it != in.end(); ++it) {
			TRACE("DeleteIncoming "<<*it);
			DeleteEdge(*it);
			TRACE("DeleteIncoming ok");
		}
	}

	void FireDeletePath(const vector<EdgeId> &edgesToDelete,
			const vector<VertexId> &verticesToDelete) {
		for (auto it = edgesToDelete.begin(); it != edgesToDelete.end(); ++it)
			FireDeleteEdge(*it);
		for (auto it = verticesToDelete.begin(); it != verticesToDelete.end();
				++it)
			FireDeleteVertex(*it);
	}

	void HiddenDeletePath(const vector<EdgeId> &edgesToDelete,
			const vector<VertexId> &verticesToDelete) {
		for (auto it = edgesToDelete.begin(); it != edgesToDelete.end(); ++it)
			HiddenDeleteEdge(*it);
		for (auto it = verticesToDelete.begin(); it != verticesToDelete.end();
				++it)
			HiddenDeleteVertex(*it);
	}

public:
	typedef typename base::SmartVertexIt SmartVertexIt;
	typedef typename base::SmartEdgeIt SmartEdgeIt;

	AbstractGraph(HandlerApplier<VertexId, EdgeId>* applier
			, const DataMaster& master) :
			base(applier), master_(master) {
	}

	virtual ~AbstractGraph() {
		TRACE("~AbstractGraph")
//		doesn't work this way because call to virtual function is needed
//		for (auto it = this->SmartVertexBegin(); !it.IsEnd(); ++it) {
//			ForceDeleteVertex(*it);
//		}
	}

	const DataMaster& master() const {
		return master_;
	}


//already in base
//	virtual const vector<EdgeId> OutgoingEdges(VertexId v) const = 0;

	virtual const vector<EdgeId> IncomingEdges(VertexId v) const = 0;

	virtual size_t OutgoingEdgeCount(VertexId v) const = 0;

	virtual size_t IncomingEdgeCount(VertexId v) const = 0;

	//todo delete if nobody needs it
	virtual vector<EdgeId> GetEdgesBetween(VertexId v, VertexId u) = 0;

	virtual const EdgeData& data(EdgeId edge) const = 0;

	virtual const VertexData& data(VertexId v) const = 0;

	virtual VertexId EdgeStart(EdgeId edge) const = 0;

	virtual VertexId EdgeEnd(EdgeId edge) const = 0;

	bool CheckUniqueOutgoingEdge(VertexId v) const {
		return OutgoingEdgeCount(v) == 1;
	}

	EdgeId GetUniqueOutgoingEdge(VertexId v) const {
		assert(CheckUniqueOutgoingEdge(v));
		return OutgoingEdges(v)[0];
	}

	bool CheckUniqueIncomingEdge(VertexId v) const {
		return IncomingEdgeCount(v) == 1;
	}

	EdgeId GetUniqueIncomingEdge(VertexId v) const {
		assert(CheckUniqueIncomingEdge(v));
		return IncomingEdges(v)[0];
	}

	std::string str(const EdgeId edge) const {
		return master_.str(data(edge));
	}

	std::string str(const VertexId v) const {
		return master_.str(data(v));
	}

	size_t length(const EdgeId edge) const {
		return master_.length(data(edge));
	}

	size_t length(const VertexId v) const {
		return master_.length(data(v));
	}

	VertexId AddVertex(const VertexData& data) {
		VertexId result = HiddenAddVertex(data);
		FireAddVertex(result);
		return result;
	}

	void DeleteVertex(VertexId v) {
		assert(IsDeadEnd(v) && IsDeadStart(v));
		assert(v != NULL);
		FireDeleteVertex(v);
		HiddenDeleteVertex(v);
	}

	void ForceDeleteVertex(VertexId v) {
		DeleteAllOutgoing(v);
		DeleteAllIncoming(v);
		DeleteVertex(v);
	}

	EdgeId AddEdge(VertexId v1, VertexId v2, const EdgeData &data) {
		EdgeId result = HiddenAddEdge(v1, v2, data);
		FireAddEdge(result);
		return result;
	}

	//todo delete if not used
	bool HasEdge(VertexId v1, VertexId v2, const EdgeData &data) {
		vector<EdgeId> out = OutgoingEdges(v1);
		for (auto it = out.begin(); it != out.end(); ++it) {
			if ((EdgeEnd(*it) == v2) && (master_.equals(data(*it), data))) {
				return true;
			}
		}
		return false;
	}

	//todo delete if not used
	EdgeId GetEdge(VertexId v1, VertexId v2, const EdgeData &edgeData) {
		vector<EdgeId> out = OutgoingEdges(v1);
		for (auto it = out.begin(); it != out.end(); ++it) {
			if ((EdgeEnd(*it) == v2) && (master_.equals(data(*it), edgeData))) {
				return *it;
			}
		}
		return NULL;
	}

	void DeleteEdge(EdgeId edge) {
		FireDeleteEdge(edge);
		HiddenDeleteEdge(edge);
	}

	bool IsDeadEnd(VertexId v) const {
		return OutgoingEdgeCount(v) == 0;
	}

	bool IsDeadStart(VertexId v) const {
		return IncomingEdgeCount(v) == 0;
	}

	bool CanCompressVertex(VertexId v) const {
		return OutgoingEdgeCount(v) == 1 && IncomingEdgeCount(v) == 1;
	}

	void CompressVertex(VertexId v) {
		//assert(CanCompressVertex(v));
		if (CanCompressVertex(v)) {
			vector<EdgeId> toMerge;
			toMerge.push_back(GetUniqueIncomingEdge(v));
			toMerge.push_back(GetUniqueOutgoingEdge(v));
			MergePath(toMerge);
		}
	}

	EdgeId MergePath(const vector<EdgeId>& path) {
		assert(!path.empty());
		vector<EdgeId> correctedPath = CorrectMergePath(path);
		SequenceBuilder sb;
		VertexId v1 = EdgeStart(correctedPath[0]);
		VertexId v2 = EdgeEnd(correctedPath[correctedPath.size() - 1]);
		vector<const EdgeData*> toMerge;
		for (auto it = correctedPath.begin(); it != correctedPath.end(); ++it) {
			toMerge.push_back(&(data(*it)));
		}
		EdgeId newEdge = HiddenAddEdge(v1, v2, master_.MergeData(toMerge));
		FireMerge(correctedPath, newEdge);
		vector<EdgeId> edgesToDelete = EdgesToDelete(path);
		vector<VertexId> verticesToDelete = VerticesToDelete(path);
		//todo ask Anton why fire and hidden are divided here
		FireDeletePath(edgesToDelete, verticesToDelete);
		FireAddEdge(newEdge);
		HiddenDeletePath(edgesToDelete, verticesToDelete);
		return newEdge;
	}

	pair<EdgeId, EdgeId> SplitEdge(EdgeId edge, size_t position) {
		pair<VertexData, pair<EdgeData, EdgeData>> newData = master_.SplitData(data(edge), position);
		VertexId splitVertex = HiddenAddVertex(newData.first);
		EdgeId newEdge1 = HiddenAddEdge(EdgeStart(edge), splitVertex,
				newData.second.first);
		EdgeId newEdge2 = HiddenAddEdge(splitVertex, EdgeEnd(edge),
				newData.second.second);
		FireSplit(edge, newEdge1, newEdge2);
		FireDeleteEdge(edge);
		FireAddVertex(splitVertex);
		FireAddEdge(newEdge1);
		FireAddEdge(newEdge2);
		HiddenDeleteEdge(edge);
		return make_pair(newEdge1, newEdge2);
	}

	void GlueEdges(EdgeId edge1, EdgeId edge2) {
		TRACE("Start glue");
		EdgeId newEdge = HiddenAddEdge(EdgeStart(edge2), EdgeEnd(edge2),
				master_.GlueData(data(edge1), data(edge2)));
		FireGlue(newEdge, edge1, edge2);
		FireDeleteEdge(edge1);
		FireDeleteEdge(edge2);
		FireAddEdge(newEdge);
		HiddenDeleteEdge(edge1);
		HiddenDeleteEdge(edge2);
		//DEBUG
//		VertexId start = EdgeStart(edge1);
//		VertexId end = EdgeEnd(edge1);
//		if (IsDeadStart(start) && IsDeadEnd(start)) {
//			WARN("Start vertex without edges after glue");
//			DeleteVertex(start);
//		}
//		if (IsDeadStart(end) && IsDeadEnd(end)) {
//			WARN("End vertex without edges after glue");
//			DeleteVertex(end);
//		}
		//DEBUG
		TRACE("End glue");

		/*/////////////////
		 FireDeleteEdge(edge2);
		 FireGlue(edge1, edge2);
		 edge2->set_data(master_.GlueData(edge1->data(), edge2->data()));
		 FireAddEdge(edge2);
		 VertexId start = EdgeStart(edge1);
		 VertexId end = EdgeEnd(edge1);
		 DeleteEdge(edge1);
		 if (IsDeadStart(start) && IsDeadEnd(start)) {
		 DeleteVertex(start);
		 }
		 if (IsDeadStart(end) && IsDeadEnd(end)) {
		 DeleteVertex(end);
		 }*/
	}

};

}
#endif /* ABSTRUCT_GRAPH_HPP_ */

