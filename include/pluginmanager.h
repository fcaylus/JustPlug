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

struct ReturnCode
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
class PluginManager
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

    static std::string appDirectory();

    size_t pluginsCount() const;
    std::vector<std::string> pluginsList() const;

    bool hasPlugin(const std::string& name) const;
    bool hasPlugin(const std::string& name, const std::string& minVersion) const;

    bool isPluginLoaded(const std::string& name) const;

    template<typename PluginType = IPlugin>
    std::shared_ptr<PluginType> pluginObject(const std::string& name);

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
