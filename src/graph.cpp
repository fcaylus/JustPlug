/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Fabien Caylus
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "private/graph.h"

using namespace jp_private;

// Constructor
Graph::Graph(const NodeList& nodeList): _nodeList(nodeList)
{

}

Graph::NodeNamesList Graph::topologicalSort(bool& error)
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

//
// Private
//

// implement depth search algorithm
bool Graph::visitNode(Node& node, NodeNamesList* list)
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
