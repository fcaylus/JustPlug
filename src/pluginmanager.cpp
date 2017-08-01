#include "pluginmanager.h"

// Used for DLL management
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/shared_library_load_mode.hpp>
#include <boost/dll/import.hpp>

// Tribool object
#include <boost/logic/tribool.hpp>

#include <iostream>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <type_traits> // for std::is_base_of

#include "version.h"
#include "json.hpp"

#include "graph.h"

using namespace jp;
using namespace boost::logic;

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
    case LOAD_DEPENDENCY_BAD_VERSION:
        return "The plugin requires a dependency that's in an incorrect version";
        break;
    case LOAD_DEPENDENCY_NOT_FOUND:
        return "THe plugin requires a dependency that wasn't found";
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
// Implementation Classes (and some typedefs)
//

typedef std::vector<boost::filesystem::path> PathList;

// Internal structure to store plugins and their associated library
struct Plugin {
    typedef std::shared_ptr<IPlugin> (iplugin_create_t)();

    std::shared_ptr<IPlugin> iplugin;
    std::function<iplugin_create_t> creator;
    boost::dll::shared_library lib;
    PluginInfo info;

    //
    // Flags used when loading
    tribool dependenciesExists = indeterminate; // true if all dependencies are present, indeterminate if not yet checked
    int graphId = -1;

    // Destructor
    ~Plugin()
    {
        // Just in case the plugins have not been unloaded (should not happen)
        if(lib.is_loaded())
        {
            if(iplugin)
                iplugin->aboutToBeUnloaded();
            iplugin.reset();
            lib.unload();
        }
    }
};

// Private implementation class
struct PluginManager::PlugMgrPrivate
{
    PlugMgrPrivate(): appDir(boost::dll::program_location().parent_path()) {}
    ~PlugMgrPrivate() {}

    std::unordered_map<std::string, Plugin> pluginsMap;
    // Contains the last load order used
    std::vector<std::string> loadOrderList;
    boost::filesystem::path appDir;

    //
    // Functions

    PathList listLibrariesInDir(const boost::filesystem::path& dir, bool recursive);
    PluginInfo parseMetadata(const char* metadata);

    ReturnCode checkDependencies(Plugin &plugin, PluginManager::callback callbackFunc);
    Graph createGraph(const Graph::NodeList& nameList);

    bool unloadPlugin(Plugin& plugin);
};

//
// Private implementations functions
//

PathList PluginManager::PlugMgrPrivate::listLibrariesInDir(const boost::filesystem::path &dir, bool recursive)
{
    try
    {
        if(boost::filesystem::exists(dir) && boost::filesystem::is_directory(dir))
        {
            PathList v;

            for(auto&& x : boost::filesystem::directory_iterator(dir))
            {
                boost::filesystem::path path = x.path();
                // Filters libraries files by suffix
                if(boost::filesystem::is_regular_file(path) && path.extension() == boost::dll::shared_library::suffix())
                    v.push_back(path);

                else if(recursive && boost::filesystem::is_directory(path))
                {
                    PathList v2 = listLibrariesInDir(path, true);
                    // Append new files to the final vector
                    v.reserve(v.capacity() + v2.size());
                    for(auto p: v2)
                        v.push_back(p);
                }
            }

            return v;
        }
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    return PathList();
}

// Parse json metadata usign json.hpp (in thirdparty/ folder)
PluginInfo PluginManager::PlugMgrPrivate::parseMetadata(const char *metadata)
{
    try
    {
        using json = nlohmann::json;
        json tree = json::parse(metadata);

        // Check API version of the plugin
        if(Version(tree.at("api").get<std::string>()).compatible("1.0.0"))
        {
            PluginInfo info;
            info.name = strdup(tree["name"].get<std::string>().c_str());
            info.prettyName = strdup(tree["prettyName"].get<std::string>().c_str());
            info.version = strdup(tree["version"].get<std::string>().c_str());
            info.author = strdup(tree["author"].get<std::string>().c_str());
            info.url = strdup(tree["url"].get<std::string>().c_str());
            info.license = strdup(tree["license"].get<std::string>().c_str());
            info.copyright = strdup(tree["copyright"].get<std::string>().c_str());

            json jsonDep = tree["dependencies"];
            std::vector<Dependency> depList;
            for(json& jdep : jsonDep)
            {
                Dependency dep;
                dep.name = strdup(jdep["name"].get<std::string>().c_str());
                dep.version = strdup(jdep["version"].get<std::string>().c_str());
                depList.push_back(dep);
            }

            if(!depList.empty())
            {
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

// Checks if the dependencies required by the plugin are present and compatible
// with the required version
// If all dependencies match, mark the plugin as "compatible"
ReturnCode PluginManager::PlugMgrPrivate::checkDependencies(Plugin &plugin, callback callbackFunc)
{
    if(!indeterminate(plugin.dependenciesExists))
        return plugin.dependenciesExists == true ? ReturnCode::SUCCESS
                                                 : (pluginsMap.count(plugin.info.name) == 0 ? ReturnCode::LOAD_DEPENDENCY_NOT_FOUND
                                                                                            : ReturnCode::LOAD_DEPENDENCY_BAD_VERSION);

    for(int i=0; i < plugin.info.dependenciesNb; ++i)
    {
        const std::string& depName = plugin.info.dependencies[i].name;
        const std::string& depVer = plugin.info.dependencies[i].version;
        // Checks if the plugin dep is compatible
        if(pluginsMap.count(depName) == 0)
        {
            plugin.dependenciesExists = false;
            if(callbackFunc)
                callbackFunc(ReturnCode::LOAD_DEPENDENCY_NOT_FOUND, strdup(plugin.lib.location().string().c_str()));
            return ReturnCode::LOAD_DEPENDENCY_NOT_FOUND;
        }

        if(!Version(pluginsMap[depName].info.version).compatible(depVer))
        {
            plugin.dependenciesExists = false;
            if(callbackFunc)
                callbackFunc(ReturnCode::LOAD_DEPENDENCY_BAD_VERSION, strdup(plugin.lib.location().string().c_str()));
            return ReturnCode::LOAD_DEPENDENCY_BAD_VERSION;
        }

        // Checks if the dependencies of the dependency exists
        ReturnCode retCode = checkDependencies(pluginsMap[depName], callbackFunc);
        if(!retCode)
            return retCode;
    }

    plugin.dependenciesExists = true;
    return ReturnCode::SUCCESS;
}

// Creates a graph that represent the structure between each plugin and its dependencies
Graph PluginManager::PlugMgrPrivate::createGraph(const Graph::NodeList& nameList)
{
    // A graph is composed of two elements:
    // - a node list (nameList)
    // - an edge list (relations between nodes, parent --> child)

    Graph::EdgeList edgeList;
    // Reserve at least nameList.size() elements
    // (there can be more or less edges, but that's a good approximation and avoid too many reallocations)
    edgeList.reserve(nameList.size()*sizeof(Graph::Edge));

    for(int i=0, iMax=nameList.size(); i<iMax; ++i)
    {
        const Plugin& plugin = pluginsMap[nameList[i]];

        for(int j=0; j<plugin.info.dependenciesNb; ++j)
        {
            int depId = pluginsMap[plugin.info.dependencies[j].name].graphId;
            edgeList.push_back(Graph::Edge(depId, i));
        }
    }

    return Graph(nameList, edgeList);
}

// Return true if the plugin is successfully unloaded
bool PluginManager::PlugMgrPrivate::unloadPlugin(Plugin &plugin)
{
    plugin.iplugin->aboutToBeUnloaded();
    plugin.iplugin.reset();
    plugin.lib.unload();

    return !plugin.lib.is_loaded();
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

ReturnCode PluginManager::searchForPlugins(const std::string &pluginDir, bool recursive, callback callbackFunc)
{
    bool atLeastOneFound = false;
    PathList libList = _p->listLibrariesInDir(pluginDir, recursive);
    for(boost::filesystem::path path : libList)
    {
        try
        {
            boost::dll::shared_library lib(path);
            if(lib.is_loaded() && lib.has("jp_name") && lib.has("jp_metadata"))
            {
                std::cout << "Found library at: " << lib.location() << std::endl;
                // This is a JustPlug library
                Plugin plugin;
                plugin.lib = lib;
                const std::string name = lib.get<const char*>("jp_name");

                // name must be unique for each plugin
                if(_p->pluginsMap.count(name) == 1)
                {
                    if(callbackFunc)
                        callbackFunc(ReturnCode::SEARCH_NAME_ALREADY_EXISTS, strdup(lib.location().string().c_str()));
                    continue;
                }

                std::cout << "Library name: " << name << std::endl;

                PluginInfo info = _p->parseMetadata(lib.get<const char[]>("jp_metadata"));
                if(!info.name)
                {
                    if(callbackFunc)
                        callbackFunc(ReturnCode::SEARCH_CANNOT_PARSE_METADATA, strdup(lib.location().string().c_str()));
                    continue;
                }

                plugin.info = info;
                std::cout << info.toString() << std::endl;

                _p->pluginsMap[name] = plugin;
                atLeastOneFound = true;
            }
        }
        // Can be thrown by shared_lib constructor in case of insufficient memory
        catch(const std::bad_alloc&)
        {
            std::cout << "Cannot load (insufficient memory): " << path << std::endl;
        }
    }

    if(atLeastOneFound)
        return ReturnCode::SUCCESS;
    return ReturnCode::SEARCH_NOTHING_FOUND;
}

ReturnCode PluginManager::searchForPlugins(const std::string &pluginDir, callback callbackFunc)
{
    return searchForPlugins(pluginDir, false, callbackFunc);
}

ReturnCode PluginManager::loadPlugins(bool tryToContinue, callback callbackFunc)
{
    // First step: For each plugins, check if it's dependencies have been found
    // and map each plugin name to an index (the index in the vector)
    std::vector<std::string> nameList;
    nameList.reserve(sizeof(std::string)*_p->pluginsMap.size());

    for(std::pair<const std::string, Plugin>& val : _p->pluginsMap)
    {
        ReturnCode retCode = _p->checkDependencies(val.second, callbackFunc);
        if(!tryToContinue && !retCode)
        {
            // An error occured, on one plugin, stop everything
            return retCode;
        }

        if(val.second.dependenciesExists)
        {
            nameList.push_back(val.first);
            val.second.graphId = nameList.size()-1;
        }
    }

    // Second step: create a graph of all dependencies
    Graph graph = _p->createGraph(nameList);

    // Third step: check for cycles in the graph
    if(graph.checkForCycle())
    {
        if(callbackFunc)
            callbackFunc(ReturnCode::LOAD_DEPENDENCY_CYCLE, nullptr);
        return ReturnCode::LOAD_DEPENDENCY_CYCLE;
    }

    // Fourth step: find the correct loading order
    _p->loadOrderList = graph.topologicalSort();
    std::cout << "Load order:" << std::endl;
    for(auto const& name : _p->loadOrderList)
        std::cout << " - " << name << std::endl;

    // Fifth step: load plugins
    for(const std::string& name : _p->loadOrderList)
    {
        Plugin& plugin = _p->pluginsMap.at(name);
        // Only load the plugin if it's not already loaded
        if(!plugin.iplugin)
        {
            // get_alias cannot be used because plugins must not depends on boost headers
            plugin.creator = *(plugin.lib.get<Plugin::iplugin_create_t*>("jp_createPlugin"));
            plugin.iplugin = plugin.creator();
            plugin.iplugin->loaded();
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
    for(std::vector<std::string>::reverse_iterator it = _p->loadOrderList.rbegin();
        it != _p->loadOrderList.rend(); ++it)
    {
        if(!_p->unloadPlugin(_p->pluginsMap[*it]))
            allUnloaded = false;
        _p->pluginsMap.erase(*it);
    }

    // Remove remaining plugins (if they are not in the loading list)
    while(!_p->pluginsMap.empty())
    {
        if(_p->unloadPlugin(_p->pluginsMap.begin()->second))
            allUnloaded = false;
        _p->pluginsMap.erase(_p->pluginsMap.begin());
    }

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

std::string PluginManager::appDirectory() const
{
    return _p->appDir.string();
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

bool PluginManager::hasPlugin(const std::string &name) const
{
    return _p->pluginsMap.count(name) == 1;
}

bool PluginManager::hasPlugin(const std::string &name, const std::string &minVersion) const
{
    return hasPlugin(name) && Version(_p->pluginsMap[name].info.version).compatible(minVersion);
}

bool PluginManager::isPluginLoaded(const std::string &name) const
{
    return hasPlugin(name) && _p->pluginsMap[name].lib.is_loaded() && _p->pluginsMap[name].iplugin;
}

template<typename PluginType>
std::shared_ptr<PluginType> PluginManager::pluginObject(const std::string& name)
{
    static_assert(std::is_base_of<IPlugin, PluginType>::value, "Plugin type must be a derived class of IPlugin");
    if(!hasPlugin(name))
        return std::shared_ptr<PluginType>();

    std::shared_ptr<IPlugin> iplugin = _p->pluginsMap[name].iplugin;
    return std::dynamic_pointer_cast<PluginType>(iplugin);
}

PluginInfo PluginManager::pluginInfo(const std::string &name) const
{
    if(hasPlugin(name))
        return _p->pluginsMap[name].info;
    return PluginInfo();
}
