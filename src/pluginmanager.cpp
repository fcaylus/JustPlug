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

#include <iostream> // for std::cout
#include <cstring> // for strdup
#include <iterator> // for std::find
#include <algorithm> // for std::copy
#include <unordered_map> // for std::unordered_map
#include <type_traits> // for std::is_base_of
#include <memory> // for std::shared_ptr and std::unique_ptr

#include "sharedlibrary.h"

#include "version/version.h"
#include "json/json.hpp"

#include "private/graph.h"
#include "private/tribool.h"
#include "private/fsutil.h"

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
/* PlugMgrPrivate class ******************************************************/
/*****************************************************************************/

//
// Implementation Classes
//

// Internal structure to store plugins and their associated library
struct Plugin
{
    typedef IPlugin* (iplugin_create_t)(_JP_MGR_REQUEST_FUNC_SIGNATURE());

    std::unique_ptr<IPlugin> iplugin;
    std::function<iplugin_create_t> creator;
    SharedLibrary lib;
    PluginInfo info;
    std::string path;

    //
    // Flags used when loading

    // true if all dependencies are present, indeterminate if not yet checked
    TriBool dependenciesExists = TriBool::Indeterminate;
    int graphId = -1;

    // Destructor
    virtual ~Plugin()
    {
        // Just in case the plugins have not been unloaded (should not happen)
        if(lib.isLoaded())
        {
            if(iplugin)
                iplugin->aboutToBeUnloaded();
            iplugin.reset();
            lib.unload();
        }

        free((char*)info.name);
        free((char*)info.prettyName);
        free((char*)info.version);
        free((char*)info.author);
        free((char*)info.url);
        free((char*)info.license);
        free((char*)info.copyright);
        free((char*)info.dependencies);
    }

    Plugin() = default;

    // Non-copyable
    Plugin(const Plugin&) = delete;
    const Plugin& operator=(const Plugin&) = delete;
};
typedef std::shared_ptr<Plugin> PluginPtr;

// Private implementation class
struct PluginManager::PlugMgrPrivate
{
    PlugMgrPrivate() {}
    ~PlugMgrPrivate() {}

    std::unordered_map<std::string, PluginPtr> pluginsMap;
    // Contains the last load order used
    std::vector<std::string> loadOrderList;

    // List all locations to load plugins
    std::vector<std::string> locations;

    //
    // Functions

    PluginInfo parseMetadata(const char* metadata);
    ReturnCode checkDependencies(PluginPtr plugin, PluginManager::callback callbackFunc);

    bool unloadPlugin(PluginPtr plugin);

    // Function called by plugins throught IPlugin::sendRequest()
    static uint16_t handleRequest(const char* sender, const char *receiver, uint16_t code, void* data, uint32_t *dataSize);
};

//
// Private implementations functions
//

// Parse json metadata usign json.hpp (in thirdparty/ folder)
PluginInfo PluginManager::PlugMgrPrivate::parseMetadata(const char *metadata)
{
    try
    {
        using json = nlohmann::json;
        json tree = json::parse(metadata);

        // Check API version of the plugin
        if(Version(tree.at("api").get<std::string>()).compatible(JP_PLUGIN_API))
        {
            PluginInfo info;
            info.name = strdup(tree.at("name").get<std::string>().c_str());
            info.prettyName = strdup(tree.at("prettyName").get<std::string>().c_str());
            info.version = strdup(tree.at("version").get<std::string>().c_str());
            info.author = strdup(tree.at("author").get<std::string>().c_str());
            info.url = strdup(tree.at("url").get<std::string>().c_str());
            info.license = strdup(tree.at("license").get<std::string>().c_str());
            info.copyright = strdup(tree.at("copyright").get<std::string>().c_str());

            json jsonDep = tree.at("dependencies");
            std::vector<Dependency> depList;
            for(json& jdep : jsonDep)
            {
                Dependency dep;
                dep.name = strdup(jdep.at("name").get<std::string>().c_str());
                dep.version = strdup(jdep.at("version").get<std::string>().c_str());
                depList.push_back(dep);
            }

            if(!depList.empty())
            {
                // Convert std::vector to C-style array used by PluginInfo
                info.dependencies = (Dependency*)std::malloc(sizeof(Dependency)*depList.size());
                std::copy(depList.begin(), depList.end(), info.dependencies);
                info.dependenciesNb = depList.size();
            }

            return info;
        }
    }
    catch(const std::exception&) {}

    return PluginInfo();
}

// Checks if the dependencies required by the plugin exists and are compatible
// with the required version.
// If all dependencies match, mark the plugin as "compatible"
ReturnCode PluginManager::PlugMgrPrivate::checkDependencies(PluginPtr plugin, callback callbackFunc)
{
    if(!plugin->dependenciesExists.indeterminate())
        return plugin->dependenciesExists == true ? ReturnCode::SUCCESS
                                                  : (pluginsMap.count(plugin->info.name) == 0 ? ReturnCode::LOAD_DEPENDENCY_NOT_FOUND
                                                                                              : ReturnCode::LOAD_DEPENDENCY_BAD_VERSION);

    for(int i=0; i < plugin->info.dependenciesNb; ++i)
    {
        const std::string& depName = plugin->info.dependencies[i].name;
        const std::string& depVer = plugin->info.dependencies[i].version;
        // Checks if the plugin dep is compatible
        if(pluginsMap.count(depName) == 0)
        {
            plugin->dependenciesExists = false;
            if(callbackFunc)
                callbackFunc(ReturnCode::LOAD_DEPENDENCY_NOT_FOUND, strdup(plugin->path.c_str()));
            return ReturnCode::LOAD_DEPENDENCY_NOT_FOUND;
        }

        if(!Version(pluginsMap[depName]->info.version).compatible(depVer))
        {
            plugin->dependenciesExists = false;
            if(callbackFunc)
                callbackFunc(ReturnCode::LOAD_DEPENDENCY_BAD_VERSION, strdup(plugin->path.c_str()));
            return ReturnCode::LOAD_DEPENDENCY_BAD_VERSION;
        }

        // Checks if the dependencies of the dependency exists
        ReturnCode retCode = checkDependencies(pluginsMap[depName], callbackFunc);
        if(!retCode)
            return retCode;
    }

    plugin->dependenciesExists = true;
    return ReturnCode::SUCCESS;
}

// Return true if the plugin is successfully unloaded
bool PluginManager::PlugMgrPrivate::unloadPlugin(PluginPtr plugin)
{
    if(plugin->iplugin)
    {
        plugin->iplugin->aboutToBeUnloaded();
        plugin->iplugin.reset();
    }
    plugin->lib.unload();
    const bool isLoaded = plugin->lib.isLoaded();
    plugin.reset();

    return !isLoaded;
}

// Static
uint16_t PluginManager::PlugMgrPrivate::handleRequest(const char *sender,
                                                      const char *receiver,
                                                      uint16_t code,
                                                      void *data,
                                                      uint32_t *dataSize)
{
    std::cout << "Request from " << sender << " !" << std::endl;

    PluginManager::PlugMgrPrivate *_p = PluginManager::instance()._p;

    // If receiver is null, the plugin manager is the receiver,
    // otherwise, re-root the request to the corresponding plugin
    if(receiver)
    {
        auto it = _p->pluginsMap.find(std::string(receiver));
        if(it != _p->pluginsMap.end())
        {
            PluginPtr plugin = it->second;
            if(plugin->lib.isLoaded() && plugin->iplugin)
            {
                return plugin->iplugin->handleRequest(sender, code, data, dataSize);
            }
        }

        // An error occured
        return 0;
    }
    // The receiver is the manager
    // TODO: handle request depending on code

    return 0;
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

ReturnCode PluginManager::searchForPlugins(const std::string &pluginDir, bool recursive, callback callbackFunc)
{
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
            std::cout << "Found library at: " << path << std::endl;
            plugin->path = path;
            std::string name = plugin->lib.get<const char*>("jp_name");;

            // name must be unique for each plugin
            if(_p->pluginsMap.count(name) == 1)
            {
                if(callbackFunc)
                    callbackFunc(ReturnCode::SEARCH_NAME_ALREADY_EXISTS, strdup(path.c_str()));
                continue;
            }

            std::cout << "Library name: " << name << std::endl;

            PluginInfo info = _p->parseMetadata(plugin->lib.get<const char[]>("jp_metadata"));
            if(!info.name)
            {
                if(callbackFunc)
                    callbackFunc(ReturnCode::SEARCH_CANNOT_PARSE_METADATA, strdup(path.c_str()));
                continue;
            }

            plugin->info = info;
            std::cout << printableInfoString(info) << std::endl;

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

ReturnCode PluginManager::loadPlugins(bool tryToContinue, callback callbackFunc)
{
    // First step: For each plugins, check if it's dependencies have been found
    // Also creates a node list used by the graph to sort the dependencies
    // NOTE: The graph is re-created even if loadPlugins() was already called.
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
            for(int i=0; i<val.second->info.dependenciesNb; ++i)
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

    std::cout << "Load order:" << std::endl;
    for(auto const& name : _p->loadOrderList)
        std::cout << " - " << name << std::endl;

    // Fourth step: load plugins
    for(const std::string& name : _p->loadOrderList)
    {
        PluginPtr plugin = _p->pluginsMap.at(name);
        // Only load the plugin if it's not already loaded
        if(!plugin->iplugin)
        {
            plugin->creator = *(plugin->lib.get<Plugin::iplugin_create_t*>("jp_createPlugin"));
            plugin->iplugin.reset(plugin->creator(PlugMgrPrivate::handleRequest));
            plugin->iplugin->loaded();
        }
    }

    return ReturnCode::SUCCESS;
}

ReturnCode PluginManager::loadPlugins(callback callbackFunc)
{
    return loadPlugins(true, callbackFunc);
}

ReturnCode PluginManager::unloadPlugins(callback callbackFunc)
{
    // Unload plugins in reverse order
    bool allUnloaded = true;
    for(auto it = _p->loadOrderList.rbegin();
        it != _p->loadOrderList.rend(); ++it)
    {
        if(!_p->unloadPlugin(_p->pluginsMap[*it]))
            allUnloaded = false;
        _p->pluginsMap.erase(*it);
    }

    // Remove remaining plugins (if they are not in the loading list)
    while(!_p->pluginsMap.empty())
    {
        if(!_p->unloadPlugin(_p->pluginsMap.begin()->second))
            allUnloaded = false;
        _p->pluginsMap.erase(_p->pluginsMap.begin());
    }

    // Clear the locations list
    _p->locations.clear();

    if(!allUnloaded)
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

template<typename PluginType>
std::shared_ptr<PluginType> PluginManager::pluginObject(const std::string& name)
{
    static_assert(std::is_base_of<IPlugin, PluginType>::value, "Plugin type must be a derived class of IPlugin");
    if(!hasPlugin(name))
        return std::shared_ptr<PluginType>();

    std::shared_ptr<IPlugin> iplugin = _p->pluginsMap[name]->iplugin;
    return std::dynamic_pointer_cast<PluginType>(iplugin);
}

PluginInfo PluginManager::pluginInfo(const std::string &name) const
{
    if(hasPlugin(name))
        return _p->pluginsMap[name]->info;
    return PluginInfo();
}

// Static
std::string PluginManager::printableInfoString(const PluginInfo &info)
{
    using namespace std;

    if(!info.name)
        return "Invalid PluginInfo";

    string str = "Plugin info:\n";
    str += "Name: " + string(info.name) + "\n";
    str += "Pretty name: " + string(info.prettyName) + "\n";
    str += "Version: " + string(info.version) + "\n";
    str += "Author: " + string(info.author) + "\n";
    str += "Url: " + string(info.url) + "\n";
    str += "License: " + string(info.license) + "\n";
    str += "Copyright: " + string(info.copyright) + "\n";
    str += "Dependencies:\n";
    for(int i=0; i < info.dependenciesNb; ++i)
        str += " - " + string(info.dependencies[i].name) + " (" + string(info.dependencies[i].version) + ")\n";
    return str;
}

std::string PluginManager::pluginPrintableInfo(const std::string &name) const
{
    return printableInfoString(pluginInfo(name));
}
