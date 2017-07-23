#ifndef GRAPH_H
#define GRAPH_H

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/visitors.hpp>

#include <vector>
#include <string>

namespace jp {

// Internal header that encapsulates all Graph-stuffs
// Mainly used to find the plugin loading order and manage dependencies
class Graph
{
public:

    typedef std::pair<int, int> Edge;
    typedef std::vector<std::string> NodeList;
    typedef std::vector<Edge> EdgeList;

    Graph(const NodeList& nodeList, const EdgeList& edgeList)
        : _nodeList(nodeList), _edgeList(edgeList)
    {
        _graph = new GraphBoost(_edgeList.begin(), _edgeList.end(), _nodeList.size());
    }

    ~Graph()
    {
        delete _graph;
    }

    NodeList topologicalSort()
    {
        if(!_graph)
            return NodeList();

        std::list<NodeBoost> nodeOrder;
        std::list<NodeBoost>::iterator i;

        NodeList list;

        boost::topological_sort(*_graph, std::front_inserter(nodeOrder));
        for(i=nodeOrder.begin(); i != nodeOrder.end(); ++i)
            list.push_back(_nodeList[*i]);
        return list;
    }

    bool checkForCycle()
    {
        bool hasCycle = false;
        CycleDetector detector(hasCycle);
        boost::depth_first_search(*_graph, boost::visitor(detector));
        return hasCycle;
    }

private:

    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> GraphBoost;
    typedef boost::graph_traits<GraphBoost>::vertex_descriptor NodeBoost;
    GraphBoost *_graph = nullptr;

    NodeList _nodeList;
    EdgeList _edgeList;

    // Visitor used to check for cycles
    struct CycleDetector : public boost::dfs_visitor<>
    {
        CycleDetector(bool& hasCycle): _hasCycle(hasCycle) {}

        template <class T_Edge, class T_Graph>
        void back_edge(T_Edge, T_Graph&)
        {
            _hasCycle = true;
        }

    private:
        bool& _hasCycle;

    };
};

} // namespace jp

#endif // GRAPH_H
