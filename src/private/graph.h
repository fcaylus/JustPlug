#ifndef GRAPH_H
#define GRAPH_H

/*
 * This file is an internal header. It's not part of the public API,
 * and may change at any moment.
 */

#include <vector>
#include <string>

namespace jp_private
{

// Internal class that encapsulates all Graph-stuffs
// Mainly used to find the plugin loading order and manage dependencies
// NOTE: The graph is altered after the sort (flags aren't reset), so only
// use it once.
class Graph
{
public:

    enum Flag
    {
        UNMARKED = 0,
        MARK_TEMP = 1,
        MARK_PERMANENT = 2
    };

    struct Node
    {
        const std::string* name;
        // Edge: parent --> this
        std::list<int> parentNodes;
        Flag flag = UNMARKED;
    };

    typedef std::vector<std::string> NodeNamesList;
    typedef std::vector<Node> NodeList;

    Graph(const NodeList& nodeList): _nodeList(nodeList)
    {
    }

    ~Graph()
    {
    }

    // This sort use a Depth-first search algorithm as described at:
    // https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
    NodeNamesList topologicalSort(bool& error)
    {
        NodeNamesList list;
        list.reserve(_nodeList.size());
        for(Node& node: _nodeList)
        {
            if(node.flag == UNMARKED)
            {
                if(!visitNode(node, &list))
                {
                    error = true;
                    return NodeNamesList();
                }
            }
        }

        error = false;
        return list;
    }

private:
    NodeList _nodeList;
    std::vector<int> _unmarkedNodes;

    bool visitNode(Node& node, NodeNamesList* list)
    {
        if(node.flag == MARK_PERMANENT)
            return true;
        else if(node.flag == MARK_TEMP)
            return false; // it's not a directed acyclic graph

        node.flag = MARK_TEMP;
        for(int parentId : node.parentNodes)
        {
            if(!visitNode(_nodeList[parentId], list))
                return false;
        }
        node.flag = MARK_PERMANENT;
        list->push_back(*(node.name));
        return true;
    }
};

} // namespace jp_private

#endif // GRAPH_H
