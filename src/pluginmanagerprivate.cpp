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

#include "private/pluginmanagerprivate.h"

#include "sharedlibrary.h"

#include "version/version.h"
#include "json/json.hpp"

#include "private/graph.h"
#include "private/tribool.h"
#include "private/fsutil.h"
#include "private/stringutil.h"

using namespace jp_private;
using namespace jp;

// Parse json metadata using json.hpp (in thirdparty/ folder)
PluginInfoStd PlugMgrPrivate::parseMetadata(const char *metadata)
{
    try
    {
        using json = nlohmann::json;
        json tree = json::parse(metadata);

        // Check API version of the plugin
        if(Version(tree.at("api").get<std::string>()).compatible(JP_PLUGIN_API))
        {
            PluginInfoStd info;
            info.name = tree.at("name").get<std::string>();
            info.prettyName = tree.at("prettyName").get<std::string>();
            info.version = tree.at("version").get<std::string>();
            info.author = tree.at("author").get<std::string>();
            info.url = tree.at("url").get<std::string>();
            info.license = tree.at("license").get<std::string>();
            info.copyright = tree.at("copyright").get<std::string>();

            json jsonDep = tree.at("dependencies");
            for(json& jdep : jsonDep)
            {
                PluginInfoStd::Dependency dep;
                dep.name = jdep.at("name").get<std::string>();
                dep.version = jdep.at("version").get<std::string>();
                info.dependencies.push_back(dep);
            }

            return info;
        }
    }
    catch(const std::exception&) {}

    return PluginInfoStd();
}

// Checks if the dependencies required by the plugin exists and are compatible
// with the required version.
// If all dependencies match, mark the plugin as "compatible"
ReturnCode PlugMgrPrivate::checkDependencies(PluginPtr& plugin, PluginManager::callback callbackFunc)
{
    if(!plugin->dependenciesExists.indeterminate())
        return plugin->dependenciesExists == true ? ReturnCode::SUCCESS
                                                  : (pluginsMap.count(plugin->info.name) == 0 ? ReturnCode::LOAD_DEPENDENCY_NOT_FOUND
                                                                                              : ReturnCode::LOAD_DEPENDENCY_BAD_VERSION);

    for(size_t i=0; i < plugin->info.dependencies.size(); ++i)
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

void PlugMgrPrivate::loadPluginsInOrder()
{
    for(const std::string& name : loadOrderList)
        loadPlugin(pluginsMap.at(name));
}

void PlugMgrPrivate::loadPlugin(PluginPtr& plugin)
{
    plugin->creator = *(plugin->lib.get<Plugin::iplugin_create_t*>("jp_createPlugin"));

    // Get a list of dependencies names and handle request functions
    const int depNb = plugin->info.dependencies.size();
    IPlugin** depPlugins = (IPlugin**)malloc(sizeof(IPlugin*)*depNb);

    // Dependencies are already loaded, so it's safe to get the plugin object
    for(int i=0; i < depNb; ++i)
        depPlugins[i] = pluginsMap[plugin->info.dependencies[i].name]->iplugin.get();

    plugin->iplugin.reset(plugin->creator(PlugMgrPrivate::handleRequest, depPlugins, depNb));
    plugin->iplugin->loaded();
}

bool PlugMgrPrivate::unloadPluginsInOrder()
{
    // Unload plugins in reverse order
    bool allUnloaded = true;
    for(auto it = loadOrderList.rbegin();
        it != loadOrderList.rend(); ++it)
    {
        if(!unloadPlugin(pluginsMap[*it]))
            allUnloaded = false;
        pluginsMap.erase(*it);
    }

    // Remove remaining plugins (if they are not in the loading list)
    while(!pluginsMap.empty())
    {
        if(!unloadPlugin(pluginsMap.begin()->second))
            allUnloaded = false;
        pluginsMap.erase(pluginsMap.begin());
    }

    // Clear the locations list
    locations.clear();

    return allUnloaded;
}

// Return true if the plugin is successfully unloaded
bool PlugMgrPrivate::unloadPlugin(PluginPtr& plugin)
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
uint16_t PlugMgrPrivate::handleRequest(const char *sender,
                                       uint16_t code,
                                       void **data,
                                       uint32_t *dataSize)
{
    // NOTE: For now, handleRequest() is thread safe if plugins don't spawn thread
    // Indeed, this method is only called from loaded() or aboutToBeUnloaded() functions
    // which are surrounded by locks.
    return handleRequestImpl(sender, code, data, dataSize);
}

uint16_t PlugMgrPrivate::handleRequestImpl(const char *sender, uint16_t code, void **data, uint32_t *dataSize)
{
    PlugMgrPrivate *_p = PluginManager::instance()._p;

    if(_p->useLog)
        _p->log.get() << "Request from " << sender << " !" << std::endl;

    // All requests to the manager sent or receive data, so check here if dataSize is null
    if(!dataSize)
        return IPlugin::DATASIZE_NULL;

    switch(code)
    {
    case IPlugin::GET_APPDIRECTORY:
    {
        *data = (void*)strdup(PluginManager::appDirectory().c_str());
        *dataSize = strlen((char*)*data);
        break;
    }
    case IPlugin::GET_PLUGINAPI:
    {
        *data = (void*)strdup(PluginManager::pluginApi().c_str());
        *dataSize = strlen((char*)*data);
        break;
    }
    case IPlugin::GET_PLUGINSCOUNT:
    {
        *data = (void*)(new size_t(PluginManager::instance().pluginsCount()));
        *dataSize = 1;
        break;
    }
    case IPlugin::GET_PLUGININFO:
    {
        PluginInfo info = PluginManager::instance().pluginInfo(*data ? (const char*)*data : sender);
        if(!info.name)
            return IPlugin::NOT_FOUND;

        *data = (void*)(new PluginInfo(info));
        *dataSize = 1;
        break;
    }
    case IPlugin::GET_PLUGINVERSION:
    {
        PluginInfo info = PluginManager::instance().pluginInfo(*data ? (const char*)*data : sender);
        if(!info.name)
            return IPlugin::NOT_FOUND;

        *data = (void*)strdup(info.version);
        *dataSize = strlen((char*)*data);
        info.free();
        break;
    }
    case IPlugin::CHECK_PLUGIN:
    {
        if(PluginManager::instance().hasPlugin((const char*)*data))
            return IPlugin::TRUE;
        return IPlugin::FALSE;
        break;
    }
    case IPlugin::CHECK_PLUGINLOADED:
    {
        if(PluginManager::instance().isPluginLoaded((const char*)*data))
            return IPlugin::TRUE;
        return IPlugin::FALSE;
        break;
    }
    default:
        return IPlugin::UNKNOWN_REQUEST;
        break;
    }

    return IPlugin::SUCCESS;
}
