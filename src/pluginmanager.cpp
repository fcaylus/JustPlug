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

#include "pluginmanager.h"

#include <algorithm> // for std::find
#include <unordered_map> // for std::unordered_map

#include "sharedlibrary.h"

#include "private/graph.h"
#include "private/fsutil.h"
#include "private/stringutil.h"
#include "private/plugin.h"

#include "version/version.h"

#include "private/pluginmanagerprivate.h"

using namespace jp;
using namespace jp_private;

/*****************************************************************************/
/* ReturnCode struct *********************************************************/
/*****************************************************************************/

ReturnCode::ReturnCode(): type(SUCCESS)
{}

ReturnCode::ReturnCode(const bool &val): type(static_cast<Type>((int)(!val)))
{}

ReturnCode::ReturnCode(const Type &codeType): type(codeType)
{}

ReturnCode::ReturnCode(const ReturnCode &code): type(code.type)
{}

ReturnCode::~ReturnCode()
{}

const ReturnCode& ReturnCode::operator=(const ReturnCode& code)
{
    type = code.type;
    return *this;
}

const char* ReturnCode::message() const
{
    return message(*this);
}

// Static
const char* ReturnCode::message(const ReturnCode &code)
{
    switch(code.type)
    {
    case SUCCESS:
        return "Success";
        break;
    case UNKNOWN_ERROR:
        return "Unknown error";
        break;
    case SEARCH_NOTHING_FOUND:
        return "No plugins was found in that directory";
        break;
    case SEARCH_CANNOT_PARSE_METADATA:
        return "Plugins metadata cannot be parsed (maybe they are invalid ?)";
        break;
    case SEARCH_NAME_ALREADY_EXISTS:
        return "A plugin with the same name was already found";
        break;
    case SEARCH_LISTFILES_ERROR:
        return "An error occurs during the scan of the plugin dir";
        break;
    case LOAD_DEPENDENCY_BAD_VERSION:
        return "The plugin requires a dependency that's in an incorrect version";
        break;
    case LOAD_DEPENDENCY_NOT_FOUND:
        return "The plugin requires a dependency that wasn't found";
        break;
    case LOAD_DEPENDENCY_CYCLE:
        return "The dependencies graph contains a cycle, which makes impossible to load plugins";
        break;
    case UNLOAD_NOT_ALL:
        return "Not all plugins have been unloaded";
        break;
    }
    return "";
}

/*****************************************************************************/
/* PluginManager class *******************************************************/
/*****************************************************************************/

PluginManager::PluginManager() : _p(new PlugMgrPrivate())
{
}

PluginManager::~PluginManager()
{
    if(!_p->pluginsMap.empty())
        unloadPlugins();
    delete _p;
}

// Static
PluginManager& PluginManager::instance()
{
    static PluginManager inst;
    return inst;
}

void PluginManager::setLogStream(std::ostream& logStream)
{
    _p->log = std::ref(logStream);
}

void PluginManager::enableLogOutput(const bool &enable)
{
    if(!_p->useLog && enable)
        _p->log.get() << "Enable log output" << std::endl;
    _p->useLog = enable;
}

void PluginManager::disableLogOutput()
{
    enableLogOutput(false);
}

ReturnCode PluginManager::searchForPlugins(const std::string &pluginDir, bool recursive, callback callbackFunc)
{
    if(_p->useLog)
        _p->log.get() << "Search for plugins in " << pluginDir << std::endl;

    bool atLeastOneFound = false;
    fsutil::PathList libList;
    if(!fsutil::listLibrariesInDir(pluginDir, &libList, recursive))
    {
        // An error occured
        if(callbackFunc)
            callbackFunc(ReturnCode::SEARCH_LISTFILES_ERROR, strerror(errno));
        // Only return if no files was found
        if(libList.empty())
            return ReturnCode::SEARCH_LISTFILES_ERROR;
    }

    for(const std::string& path : libList)
    {
        PluginPtr plugin(new Plugin());
        plugin->lib.load(path);

        if(plugin->lib.isLoaded()
           && plugin->lib.hasSymbol("jp_name")
           && plugin->lib.hasSymbol("jp_metadata")
           && plugin->lib.hasSymbol("jp_createPlugin"))
        {
            // This is a JustPlug library
            if(_p->useLog)
                _p->log.get() << "Found library at: " << path << std::endl;
            plugin->path = path;
            std::string name = plugin->lib.get<const char*>("jp_name");;

            // name must be unique for each plugin
            if(_p->pluginsMap.count(name) == 1)
            {
                if(callbackFunc)
                    callbackFunc(ReturnCode::SEARCH_NAME_ALREADY_EXISTS, strdup(path.c_str()));
                plugin.reset();
                continue;
            }

            if(_p->useLog)
                _p->log.get() << "Library name: " << name << std::endl;

            PluginInfoStd info = _p->parseMetadata(plugin->lib.get<const char[]>("jp_metadata"));
            if(info.name.empty())
            {
                if(callbackFunc)
                    callbackFunc(ReturnCode::SEARCH_CANNOT_PARSE_METADATA, strdup(path.c_str()));
                plugin.reset();
                continue;
            }

            plugin->info = info;
            // Print plugin's info
            if(_p->useLog)
                _p->log.get() << info.toString() << std::endl;

            _p->pluginsMap[name] = plugin;
            atLeastOneFound = true;
        }
        else
        {
            plugin.reset();
        }
    }

    if(atLeastOneFound)
    {
        // Only add the location if it's not already in the list
        if(std::find(_p->locations.begin(), _p->locations.end(), pluginDir) == _p->locations.end())
            _p->locations.push_back(pluginDir);
        return ReturnCode::SUCCESS;
    }
    return ReturnCode::SEARCH_NOTHING_FOUND;
}

ReturnCode PluginManager::searchForPlugins(const std::string &pluginDir, callback callbackFunc)
{
    return searchForPlugins(pluginDir, false, callbackFunc);
}

ReturnCode PluginManager::registerMainPlugin(const std::string &pluginName)
{
    if(_p->mainPluginName.empty() && hasPlugin(pluginName))
    {
        _p->mainPluginName = pluginName;
        return ReturnCode::SUCCESS;
    }
    return ReturnCode::UNKNOWN_ERROR;
}

ReturnCode PluginManager::loadPlugins(bool tryToContinue, callback callbackFunc)
{
    // First step: For each plugins, check if it's dependencies have been found
    // Also creates a node list used by the graph to sort the dependencies
    // NOTE: The graph is re-created even if loadPlugins() was already called.

    if(_p->useLog)
        _p->log.get() << "Load plugins ..." << std::endl;

    Graph::NodeList nodeList;
    nodeList.reserve(_p->pluginsMap.size());

    for(auto& val : _p->pluginsMap)
    {
        // Init the ID to the default value (in case loadPlugins is called several times)
        val.second->graphId = -1;

        ReturnCode retCode = _p->checkDependencies(val.second, callbackFunc);
        if(!tryToContinue && !retCode)
        {
            // An error occured on one plugin, stop everything
            return retCode;
        }

        if(val.second->dependenciesExists == true)
        {
            Graph::Node node;
            node.name = &(val.first);
            nodeList.push_back(node);
            val.second->graphId = nodeList.size() - 1;
        }
    }

    // Fill parentNodes list for each node
    for(auto& val : _p->pluginsMap)
    {
        const int nodeId = val.second->graphId;
        if(nodeId != -1)
        {
            for(size_t i=0; i<val.second->info.dependencies.size(); ++i)
                nodeList[nodeId].parentNodes.push_back(_p->pluginsMap[val.second->info.dependencies[i].name]->graphId);
        }
    }


    // Second step: create a graph of all dependencies
    Graph graph(nodeList);

    // Third step: find the correct loading order using the topological Sort
    bool error = false;
    _p->loadOrderList = graph.topologicalSort(error);
    if(error)
    {
        // There is a cycle inside the graph
        if(callbackFunc)
            callbackFunc(ReturnCode::LOAD_DEPENDENCY_CYCLE, nullptr);
        return ReturnCode::LOAD_DEPENDENCY_CYCLE;
    }

    if(_p->useLog)
    {
        _p->log.get() << "Load order:" << std::endl;
        for(auto const& name : _p->loadOrderList)
            _p->log.get() << " - " << name << std::endl;
    }

    // Fourth step: load plugins
    _p->loadPluginsInOrder();

    // Call the main plugin function
    if(!_p->mainPluginName.empty())
        _p->pluginsMap.at(_p->mainPluginName)->iplugin->mainPluginExec();

    // Here, all plugins are loaded, the function can return
    return ReturnCode::SUCCESS;
}

ReturnCode PluginManager::loadPlugins(callback callbackFunc)
{
    return loadPlugins(true, callbackFunc);
}

ReturnCode PluginManager::unloadPlugins(callback callbackFunc)
{
    if(_p->useLog)
        _p->log.get() << "Unload plugins ..." << std::endl;

    if(!_p->unloadPluginsInOrder())
    {
        if(callbackFunc)
            callbackFunc(ReturnCode::UNLOAD_NOT_ALL, nullptr);

        return ReturnCode::UNLOAD_NOT_ALL;
    }
    return ReturnCode::SUCCESS;
}

//
// Getters
//

// Static
std::string PluginManager::appDirectory()
{
    return fsutil::appDir();
}

// Static
std::string PluginManager::pluginApi()
{
    return JP_PLUGIN_API;
}

size_t PluginManager::pluginsCount() const
{
    return _p->pluginsMap.size();
}

std::vector<std::string> PluginManager::pluginsList() const
{
    std::vector<std::string> nameList;
    nameList.reserve(_p->pluginsMap.size());
    for(auto const& x : _p->pluginsMap)
        nameList.push_back(x.first);
    return nameList;
}

std::vector<std::string> PluginManager::pluginsLocation() const
{
    return _p->locations;
}

bool PluginManager::hasPlugin(const std::string &name) const
{
    return _p->pluginsMap.count(name) == 1;
}

bool PluginManager::hasPlugin(const std::string &name, const std::string &minVersion) const
{
    return hasPlugin(name) && Version(_p->pluginsMap[name]->info.version).compatible(minVersion);
}

bool PluginManager::isPluginLoaded(const std::string &name) const
{
    return hasPlugin(name) && _p->pluginsMap[name]->lib.isLoaded() && _p->pluginsMap[name]->iplugin;
}

std::shared_ptr<IPlugin> PluginManager::pluginObject(const std::string& name) const
{
    if(!hasPlugin(name))
        return std::shared_ptr<IPlugin>();

    return _p->pluginsMap[name]->iplugin;
}

PluginInfo PluginManager::pluginInfo(const std::string &name) const
{
    if(!hasPlugin(name))
        return PluginInfo();
    return _p->pluginsMap[name]->info.toPluginInfo();
}
