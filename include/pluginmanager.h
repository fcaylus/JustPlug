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
#include <ostream> // for std::ostream

#include "plugininfo.h"
#include "iplugin.h"

namespace jp
{

/**
 * @brief The ReturnCode struct
 * Every member functions of PluginManager returns a ReturnCode object.
 * This object can be use to identify the error.
 * Implicit cast to bool allows easy checks of functions' success.
 *
 * To get a more meaningful message about the error, meessage() can be used.
 */
struct JP_EXPORT_SYMBOL ReturnCode
{
    /**
     * @brief Enum of all possible errors
     * @todo Document each error
     */
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
    /**
     * @brief The type of the error (the error code)
     */
    Type type;

    /**
     * @brief Get a meaningful error message.
     */
    const char* message() const;
    /**
     * @brief Get a meaningful error message for a specified @a code
     */
    static const char* message(const ReturnCode& code);

    explicit operator bool() { return type == Type::SUCCESS; }

    //
    // Constructors

    /**
     * @brief Default constructor
     * Set type to Type::SUCCESS.
     */
    ReturnCode();
    /**
     * @brief Constructor
     * Set type to Type::SUCCESS if val is true, else set to Type::UNKNOWN_ERROR
     * @param val
     */
    ReturnCode(const bool& val);
    /**
     * @brief Constructor
     * Set type to codeType
     * @param codeType
     */
    ReturnCode(const Type& codeType);
    /**
     * @brief Copy constructor
     */
    ReturnCode(const ReturnCode& code);
    /**
     * @brief operator =
     */
    const ReturnCode& operator=(const ReturnCode& code);

    /**
     * @brief Default constructor
     */
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
     * Callback functions must accept two parameters:
     *  - const ReturnCode& returnCode
     *  - const char* errorDetails (may be null and MUST be free by the receiver if not)
     */
    typedef std::function<void(const ReturnCode&, const char*)> callback;

    /**
     * @brief Enable log output
     * If @a enable is true, the manager will ouput log information to the stream specified
     * by setLogStream or to std::cout by default.
     * If the user wants to disable all log outputs (to speed up the program ?), call this function
     * with @a enable set to false.
     * @param enable
     * @see setLogStream(), disableLogOutput()
     */
    void enableLogOutput(const bool& enable = true);
    /**
     * @brief Disable log output
     * Same as enableLogOutput(false)
     * @see enableLogOutput()
     */
    void disableLogOutput();

    /**
     * @brief Set stream to output log information.
     * @a logStream will be used to output every log information.
     * By default, std::cout is used.
     * @param logStream The stream to use
     * @see enableLogOutput()
     */
    void setLogStream(std::ostream &logStream);

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
     * @brief Get the plugin API.
     * This is the API of the plugin's interfaces (IPlugin, ...).
     * The version follows the Semantic Versioning 2.0.0 (http://semver.org/spec/v2.0.0.html).
     * @note ABI compatibility is only guaranteed for the same MAJOR version.
     */
    static std::string pluginApi();

    /**
     * @brief Get the number of plugins found*
     * @complexity Constant
     */
    size_t pluginsCount() const;
    /**
     * @brief Get a list of all plugins.
     * @complexity Linear in the number of plugins
     */
    std::vector<std::string> pluginsList() const;

    /**
     * @brief Get a list of all locations where plugins were found.
     * @complexity Constant
     */
    std::vector<std::string> pluginsLocation() const;

    /**
     * @brief Checks if a plugin exists.
     * @complexity Constant on average, worst case linear in the number of plugins
     * @param name The name of the plugin
     */
    bool hasPlugin(const std::string& name) const;
    /**
     * @brief Checks if a plugin exists and is compatible with @a minVersion.
     * @complexity Same complexity as hasPlugin(const std::string& name).
     */
    bool hasPlugin(const std::string& name, const std::string& minVersion) const;

    /**
     * @brief Checks if a plugin is loaded.
     * @complexity Constant on average, worst case linear in the number of plugins
     * @param name The plugin's name
     * @return true if the plugin is loaded, else returns false (not loaded or not found)
     */
    bool isPluginLoaded(const std::string& name) const;

    /**
     * @brief Get the plugin object for the specified plugin.
     * @note The user can cast the object to the corresponding type if he wants.
     * @return The object or NULL if the plugin is not loaded.
     */
    std::shared_ptr<IPlugin> pluginObject(const std::string& name) const;

    /**
     * @brief Get the PluginInfo object for the specified plugin
     * @note It's the responsability of the user to free the strings contained by PluginInfo.
     * @param name
     * @return The PluginInfo object.
     */
    PluginInfo pluginInfo(const std::string& name) const;

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
