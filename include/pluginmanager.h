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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string> // for std::string
#include <vector> // for std::vector
#include <functional> // for std::function
#include <memory> // for std::shared_ptr

#include "plugininfo.h"
#include "iplugin.h"

namespace jp
{

struct JP_EXPORT_SYMBOL ReturnCode
{
    enum Type
    {
        SUCCESS = 0,
        UNKNOWN_ERROR = 1,

        // Raised by searchForPlugins()
        SEARCH_NOTHING_FOUND = 100,
        SEARCH_NAME_ALREADY_EXISTS = 101,
        SEARCH_CANNOT_PARSE_METADATA = 102,
        SEARCH_LISTFILES_ERROR = 103,

        // Raised by loadPlugins()
        LOAD_DEPENDENCY_BAD_VERSION = 200,
        LOAD_DEPENDENCY_NOT_FOUND = 201,
        LOAD_DEPENDENCY_CYCLE = 202,

        // Raised by unloadPlugins()
        UNLOAD_NOT_ALL = 300
    };
    Type type;

    const char* message() const;
    static const char* message(const ReturnCode& code);

    explicit operator bool() { return type == Type::SUCCESS; }

    // Constructors
    ReturnCode(); // Default to SUCCESS
    ReturnCode(const bool& val); // Default to SUCCESS or UNKNOWN_ERROR depending on val
    ReturnCode(const Type& codeType);
    ReturnCode(const ReturnCode& code); // Copy constructor
    // Destructor
    ~ReturnCode();
};

/**
 * @class PluginManager
 * @brief Main class to manage all plugins.
 * Since it is a singleton-class, only one instance can be created.
 */
class JP_EXPORT_SYMBOL PluginManager
{

public:

    /**
     * @brief Return an instance of the plugin manager.
     * Calling instance() several times at different places always return a reference
     * to the same object. This allow the program to manage plugins and access informations
     * from different places.
     * @return A reference of the object.
     */
    static PluginManager& instance();

    /**
     * @brief Signature for all callback functions used for error report in this class
     * Callback functions must have two parameters:
     *  - const ReturnCode& returnCode
     *  - const char* errorDetails (may be null and MUST be free by the receiver if not)
     */
    typedef std::function<void(const ReturnCode&, const char*)> callback;

    /**
     * @brief Search for all JustPlug plugins in pluginDir.
     * This function only load the librairies in order to retrieve the metadata.
     * To actually load the "plugin object" and launch it, you must call loadPlugins() after.
     * @note This function can be called several times if plugins are in different dirs.
     * @param pluginDir The dir to search for plugin
     * @param recursive Tell if the function should search inside sub-directories.
     * @param callbackFunc This function is called upon each error during the search (if specified)
     * @return true if at least one plugin was found
     */
    ReturnCode searchForPlugins(const std::string& pluginDir, bool recursive, callback callbackFunc);
    /**
     * @brief Overloaded function
     * Same as searchForPlugins(const std::string& pluginDir, bool recursive, callback callbackFunc)
     * with recursive set to false.
     * @param pluginDir
     * @param callbackFunc
     */
    ReturnCode searchForPlugins(const std::string& pluginDir, callback callbackFunc = callback());

    /**
     * @brief Load all plugins found by previous searchForPlugins().
     * @param tryToContinue If true, the manager will try to load other plugins if some have errors.
     * @param callbackFunc Callback function.
     * @return true if all plugins was successfully loaded.
     */
    ReturnCode loadPlugins(bool tryToContinue, callback callbackFunc);
    /**
     * @brief Overloaded function
     * Same as loadPlugins(bool tryToContinue, callback callbackFunc) with tryToContinue set to true.
     * @param callbackFunc
     */
    ReturnCode loadPlugins(callback callbackFunc = callback());

    /**
     * @brief Unload all loaded plugins.
     * After this function, if the user wants to reload the plugins, he must recall searchForPlugins first.
     * @param callbackFunc Callback function.
     * @return true if all plugins are successfully unloaded.
     */
    ReturnCode unloadPlugins(callback callbackFunc = callback());

    //
    // Getters
    //

    /**
     * @brief Returns the directory path of the main application
     * @return a path to the application's directory
     */
    static std::string appDirectory();

    /**
     * @brief Convert the plugin information struct to a printable string
     * @param info The PluginInfo object to print
     * @return the printable string representation
     * @note This function may be slow due to the internal conversion to std::string.
     */
    static std::string printableInfoString(const PluginInfo& info);

    /**
     * @brief Get the plugin API.
     * This is the API of the plugin's interfaces (IPlugin, ...).
     * The version follows the Semantic Versioning 2.0.0 (http://semver.org/spec/v2.0.0.html).
     * @note ABI compatibility is only guaranteed for a specified MAJOR version.
     * @return
     */
    static std::string pluginApi();

    /**
     * @brief Get the number of plugins found*
     * @par Complexity:
     * Constant
     * @return
     */
    size_t pluginsCount() const;
    /**
     * @brief Get a list of all plugins.
     * @par Complexity:
     * Linear in the number of plugins
     * @return
     */
    std::vector<std::string> pluginsList() const;

    /**
     * @brief Get a list of all locations where plugins were found.
     * @return
     */
    std::vector<std::string> pluginsLocation() const;

    /**
     * @brief Checks if a plugin exists.
     * @par Complexity:
     * Constant on average, worst case linear in the number of plugins
     * @param name The name of the plugin
     * @return
     */
    bool hasPlugin(const std::string& name) const;
    /**
     * @brief Checks if a plugin exists and is compatible with @a minVersion.
     * Same complexity as hasPlugin(const std::string& name).
     * @return
     */
    bool hasPlugin(const std::string& name, const std::string& minVersion) const;

    /**
     * @brief Checks if a plugin is loaded.
     * @par Complexity:
     * Constant on average, worst case linear in the number of plugins
     * @param name The plugin's name
     * @return true if the plugin is loaded, else returns false (not loaded or not found)
     */
    bool isPluginLoaded(const std::string& name) const;

    /**
     * @brief Get the plugin Object for the specified plugin.
     * @return The object or NULL if the plugin is not loaded or not of the type PluginType.
     */
    template<typename PluginType = IPlugin>
    std::shared_ptr<PluginType> pluginObject(const std::string& name);

    /**
     * @brief Get the PluginInfo object for the specified plugin
     * @param name
     * @return The PluginInfo object.
     */
    PluginInfo pluginInfo(const std::string& name) const;

    /**
     * @brief Returns a printable string of specified plugin information
     * @note Same as PluginManager::printableInfoString(PluginManager::instance().pluginInfo(name))
     * @param name The plugin name
     * @return The printable string
     */
    std::string pluginPrintableInfo(const std::string& name) const;

private:
    PluginManager();
    ~PluginManager();

    // Non-copyable
    PluginManager(const PluginManager&) = delete;
    const PluginManager& operator=(const PluginManager&) = delete;

    struct PlugMgrPrivate;
    PlugMgrPrivate* const _p;
    friend struct PlugMgrPrivate;
};

} // namespace jp

#endif // PLUGINMANAGER_H
