#ifndef MOD_LIB_RULE_PROP_H
#define	MOD_LIB_RULE_PROP_H

#include <mod/lib/IO/IO.h>
#include <mod/lib/GraphUtil.h>

#include <jla_boost/graph/PairToRangeAdaptor.hpp>
#include <jla_boost/graph/dpo/Rule.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/optional/optional.hpp>

#include <vector>

namespace mod {
namespace lib {
namespace Rule {
using jla_boost::GraphDPO::Membership;

#define MOD_RULE_STATE_TEMPLATE_PARAMS											\
	template<typename Derived, typename Graph,									\
	typename LeftVertexType, typename LeftEdgeType,								\
	typename RightVertexType, typename RightEdgeType>										
#define MOD_RULE_STATE_TEMPLATE_ARGS											\
	Derived, Graph, LeftVertexType, LeftEdgeType, RightVertexType, RightEdgeType

namespace detail {

MOD_RULE_STATE_TEMPLATE_PARAMS
struct LeftState;

MOD_RULE_STATE_TEMPLATE_PARAMS
struct RightState;

} // namespace detail

template<typename Derived, typename Graph, typename LeftVertexType, typename LeftEdgeType,
typename RightVertexType = LeftVertexType, typename RightEdgeType = LeftEdgeType>
struct PropCore {
	using This = PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>;
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	using Edge = typename boost::graph_traits<Graph>::edge_descriptor;
	using LeftType = detail::LeftState<MOD_RULE_STATE_TEMPLATE_ARGS>;
	using RightType = detail::RightState<MOD_RULE_STATE_TEMPLATE_ARGS>;
	friend class detail::LeftState<MOD_RULE_STATE_TEMPLATE_ARGS>;
	friend class detail::RightState<MOD_RULE_STATE_TEMPLATE_ARGS>;
	using ValueTypeVertex = std::pair<boost::optional<const LeftVertexType&>, boost::optional<const RightVertexType&> >;
	using ValueTypeEdge = std::pair<boost::optional<const LeftEdgeType&>, boost::optional<const RightEdgeType&> >;
public:
	PropCore(const This&) = delete;
	PropCore(This&&) = delete;
	This &operator=(const This&) = delete;
	This &operator=(This&&) = delete;
public:
	void verify(const Graph *g) const;
	explicit PropCore(const Graph &g);
	ValueTypeVertex operator[](Vertex v) const;
	ValueTypeEdge operator[](Edge e) const;
	LeftType getLeft() const;
	RightType getRight() const;
	void add(Vertex v, const LeftVertexType &valueLeft, const RightVertexType &valueRight);
	void add(Edge e, const LeftEdgeType &valueLeft, const RightEdgeType &valueRight);
	// does not modify the other side
	void setLeft(Vertex v, const LeftVertexType &value);
	void setRight(Vertex v, const RightVertexType &value);
	void setLeft(Edge e, const LeftEdgeType &value);
	void setRight(Edge e, const RightEdgeType &value);
	bool isChanged(Vertex v) const;
	bool isChanged(Edge e) const;
	void print(std::ostream &s, Vertex v) const;
	void print(std::ostream &s, Edge e) const;
	const Derived &getDerived() const;
protected:
	const Graph &g;

	struct VertexStore {
		VertexStore() = default;

		VertexStore(const LeftVertexType &left, const RightVertexType &right) : left(left), right(right) { }
		LeftVertexType left;
		RightVertexType right;
	};

	struct EdgeStore {
		EdgeStore() = default;

		EdgeStore(const LeftEdgeType &left, const RightEdgeType &right) : left(left), right(right) { }
		LeftEdgeType left;
		RightEdgeType right;
	};

	std::vector<VertexStore> vertexState;
	EdgeMap<Graph, EdgeStore> edgeState;
public:

	struct Handler {

		template<typename F, typename ...Args>
		bool operator()(F &&f, const ValueTypeVertex &l, const ValueTypeVertex &r, Args&&... args) const {
			assert(l.first.is_initialized() == r.first.is_initialized());
			assert(l.second.is_initialized() == r.second.is_initialized());
			if(l.first.is_initialized() && !f(*l.first, *r.first, args...))
				return false;
			if(l.second.is_initialized())
				return f(*l.second, *r.second, args...);
			return true;
		}
	};
};

template<typename Derived, typename Graph, typename LeftVertexType, typename LeftEdgeType, typename RightVertexType, typename RightEdgeType,
typename VertexOrEdge>
auto get(const PropCore<MOD_RULE_STATE_TEMPLATE_ARGS> &p, VertexOrEdge ve) -> decltype(p[ve]) {
	return p[ve];
}

namespace detail {

MOD_RULE_STATE_TEMPLATE_PARAMS
struct LeftState {

	LeftState(const PropCore<MOD_RULE_STATE_TEMPLATE_ARGS> &state) : state(state) { }

	const LeftVertexType &operator[](typename boost::graph_traits<Graph>::vertex_descriptor v) const {
		auto vId = get(boost::vertex_index_t(), state.g, v);
		assert(vId < state.vertexState.size());
		assert(state.g[v].membership != Membership::Right);
		return state.vertexState[vId].left;
	}

	const LeftEdgeType &operator[](typename boost::graph_traits<Graph>::edge_descriptor e) const {
		assert(state.g[e].membership != Membership::Right);
		auto iter = state.edgeState.find(e);
		assert(iter != end(state.edgeState));
		return iter->second.left;
	}
public:
	const PropCore<MOD_RULE_STATE_TEMPLATE_ARGS> &state;
public:

	struct Handler {

		template<typename F, typename ...Args>
		bool operator()(F &&f, const LeftVertexType &l, const LeftVertexType &r, Args&&... args) const {
			return std::forward<F>(f) (l, r, std::forward<Args>(args)...);
		}
	};
};

MOD_RULE_STATE_TEMPLATE_PARAMS
struct RightState {

	RightState(const PropCore<MOD_RULE_STATE_TEMPLATE_ARGS> &state) : state(state) { }

	const RightVertexType &operator[](typename boost::graph_traits<Graph>::vertex_descriptor v) const {
		auto vId = get(boost::vertex_index_t(), state.g, v);
		assert(vId < state.vertexState.size());
		assert(state.g[v].membership != Membership::Left);
		return state.vertexState[vId].right;
	}

	const RightEdgeType &operator[](typename boost::graph_traits<Graph>::edge_descriptor e) const {
		assert(state.g[e].membership != Membership::Left);
		auto iter = state.edgeState.find(e);
		assert(iter != end(state.edgeState));
		return iter->second.right;
	}
public:
	const PropCore<MOD_RULE_STATE_TEMPLATE_ARGS> &state;
public:

	struct Handler {

		template<typename F, typename ...Args>
		bool operator()(F &&f, const RightVertexType &l, const RightVertexType &r, Args&&... args) const {
			return std::forward<F>(f) (l, r, std::forward<Args>(args)...);
		}
	};
};

template<typename Derived, typename Graph, typename LeftVertexType, typename LeftEdgeType, typename RightVertexType, typename RightEdgeType,
typename VertexOrEdge>
auto get(const LeftState<MOD_RULE_STATE_TEMPLATE_ARGS> &p, VertexOrEdge ve) -> decltype(p[ve]) {
	return p[ve];
}

template<typename Derived, typename Graph, typename LeftVertexType, typename LeftEdgeType, typename RightVertexType, typename RightEdgeType,
typename VertexOrEdge>
auto get(const RightState<MOD_RULE_STATE_TEMPLATE_ARGS> &p, VertexOrEdge ve) -> decltype(p[ve]) {
	return p[ve];
}

} // namespace detail

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::verify(const Graph *g) const {
	if(g != &this->g) {
		IO::log() << "Different graphs: g = " << static_cast<const void*> (g) << ", &this->g = " << static_cast<const void*> (&this->g) << std::endl;
		MOD_ABORT;
	}
	if(num_vertices(this->g) != vertexState.size()) {
		IO::log() << "Different sizes: num_vertices(this->g) = " << num_vertices(this->g) << ", vertexLabels.size() = " << vertexState.size() << std::endl;
		MOD_ABORT;
	}
	if(num_edges(this->g) != edgeState.size()) {
		IO::log() << "Different sizes: num_edges(this->g) = " << num_edges(this->g) << ", edgeLabels.size() = " << edgeState.size() << std::endl;
		MOD_ABORT;
	}
	for(Edge e : asRange(edges(this->g))) {
		auto iter = edgeState.find(e);
		if(iter == end(edgeState)) {
			IO::log() << "Edge " << e << " not found in edgeLabels:";
			for(const auto &p : edgeState) IO::log() << " " << p.first << "('" << p.second.left << "', '" << p.second.right << "')";
			IO::log() << std::endl;
			MOD_ABORT;
		}
	}
}

MOD_RULE_STATE_TEMPLATE_PARAMS
PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::PropCore(const Graph &g) : g(g), edgeState(g) { }

MOD_RULE_STATE_TEMPLATE_PARAMS
std::pair<boost::optional<const LeftVertexType&>, boost::optional<const RightVertexType&> >
PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::operator[](typename boost::graph_traits<Graph>::vertex_descriptor v) const {
	assert(v != boost::graph_traits<Graph>::null_vertex());
	boost::optional<const LeftVertexType&> l;
	boost::optional<const RightVertexType&> r;
	if(g[v].membership != Membership::Right) l = getLeft()[v];
	if(g[v].membership != Membership::Left) r = getRight()[v];
	return std::make_pair(l, r);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
std::pair<boost::optional<const LeftEdgeType&>, boost::optional<const RightEdgeType&> >
PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::operator[](typename boost::graph_traits<Graph>::edge_descriptor e) const {
	boost::optional<const LeftVertexType&> l;
	boost::optional<const RightVertexType&> r;
	if(g[e].membership != Membership::Right) l = getLeft()[e];
	if(g[e].membership != Membership::Left) r = getRight()[e];
	return std::make_pair(l, r);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
typename detail::LeftState<MOD_RULE_STATE_TEMPLATE_ARGS>
PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::getLeft() const {
	return detail::LeftState<MOD_RULE_STATE_TEMPLATE_ARGS>(*this);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
typename detail::RightState<MOD_RULE_STATE_TEMPLATE_ARGS>
PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::getRight() const {
	return detail::RightState<MOD_RULE_STATE_TEMPLATE_ARGS>(*this);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::add(Vertex v, const LeftVertexType &valueLeft, const RightVertexType &valueRight) {
	assert(num_vertices(g) == vertexState.size() + 1);
	assert(get(boost::vertex_index_t(), g, v) == vertexState.size());
	vertexState.emplace_back(valueLeft, valueRight);
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::add(Edge e, const LeftEdgeType &valueLeft, const RightEdgeType &valueRight) {
	assert(num_edges(g) == edgeState.size() + 1);
	assert(edgeState.find(e) == end(edgeState));
	edgeState.add(e, EdgeStore{valueLeft, valueRight});
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::setLeft(Vertex v, const LeftVertexType &value) {
	auto vId = get(boost::vertex_index_t(), g, v);
	assert(vId < vertexState.size());
	assert(g[v].membership != Membership::Right);
	vertexState[vId].left = value;
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::setRight(Vertex v, const RightVertexType &value) {
	auto vId = get(boost::vertex_index_t(), g, v);
	assert(vId < vertexState.size());
	assert(g[v].membership != Membership::Left);
	vertexState[vId].right = value;
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::setLeft(Edge e, const LeftEdgeType &value) {
	auto iter = edgeState.find(e);
	assert(iter != end(edgeState));
	assert(g[e].membership != Membership::Right);
	iter->second.left = value;
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::setRight(Edge e, const RightEdgeType &value) {
	auto iter = edgeState.find(e);
	assert(iter != end(edgeState));
	assert(g[e].membership != Membership::Left);
	iter->second.right = value;
	verify(&g);
}

MOD_RULE_STATE_TEMPLATE_PARAMS
bool PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::isChanged(Vertex v) const {
	if(g[v].membership != Membership::Context) return false;
	auto vId = get(boost::vertex_index_t(), g, v);
	assert(vId < vertexState.size());
	return vertexState[vId].left != vertexState[vId].right;
}

MOD_RULE_STATE_TEMPLATE_PARAMS
bool PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::isChanged(Edge e) const {
	if(g[e].membership != Membership::Context) return false;
	auto iter = edgeState.find(e);
	assert(iter != end(edgeState));
	return iter->second.left != iter->second.right;
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::print(std::ostream &s, Vertex v) const {
	assert(get(boost::vertex_index_t(), g, v) < vertexState.size());
	auto membership = g[v].membership;
	if(membership == Membership::Right) s << "<>";
	else s << '\'' << vertexState[get(boost::vertex_index_t(), g, v)].left << '\'';
	s << " -> ";
	if(membership == Membership::Left) s << "<>";
	else s << '\'' << vertexState[get(boost::vertex_index_t(), g, v)].right << '\'';
}

MOD_RULE_STATE_TEMPLATE_PARAMS
void PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::print(std::ostream &s, Edge e) const {
	auto iter = edgeState.find(e);
	assert(iter != end(edgeState));
	auto membership = g[e].membership;
	if(membership == Membership::Right) s << "<>";
	else s << '\'' << iter->second.left << '\'';
	s << " -> ";
	if(membership == Membership::Left) s << "<>";
	else s << '\'' << iter->second.right << '\'';
}

MOD_RULE_STATE_TEMPLATE_PARAMS
const Derived &PropCore<MOD_RULE_STATE_TEMPLATE_ARGS>::getDerived() const {
	return static_cast<const Derived&> (*this);
}

} // namespace Rule
} // namespace lib
} // namespace mod

#endif	/* MOD_LIB_RULE_PROP_H */