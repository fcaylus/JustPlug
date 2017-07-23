#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string> // for std::string
#include <functional> // for std::function
#include <memory> // for std::shared_ptr

#include "plugininfo.h"
#include "iplugin.h"

namespace jp {

struct ReturnCode {
    enum Type {
        SUCCESS = 0,
        UNKNOWN_ERROR = 1,

        // Raised by searchForPlugins()
        SEARCH_NOTHING_FOUND = 100,
        SEARCH_NAME_ALREADY_EXISTS = 101,
        SEARCH_CANNOT_PARSE_METADATA = 102,

        // Raised by loadPlugins()
        LOAD_DEPENDENCY_BAD_VERSION = 200,
        LOAD_DEPENDENCY_NOT_FOUND = 201,
        LOAD_DEPENDENCY_CYCLE = 202,

        // Raised by unloadPlugins()
        UNLOAD_NOT_ALL = 300
    };
    Type type;

    const char* message();
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
 * @brief Main class to manage all plugins
 */
class PluginManager
{

public:
    PluginManager();
    ~PluginManager();

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
    ReturnCode searchForPlugins(const std::string& pluginDir, bool recursive = false, callback* callbackFunc = nullptr);
    /**
     * @brief Load all plugins found by previous searchForPlugins().
     * @param tryToContinue If true, the manager will try to load other plugins if some have errors.
     * @param callbackFunc Callback function.
     * @return true if all plugins was successfully loaded.
     */
    ReturnCode loadPlugins(bool tryToContinue = true, callback* callbackFunc = nullptr);
    /**
     * @brief Unload all loaded plugins.
     * After this function, if the user wants to reload the plugins, he must recall searchForPlugins first.
     * @param callbackFunc Callback function.
     * @return true if all plugins are successfully unloaded.
     */
    ReturnCode unloadPlugins(callback* callbackFunc = nullptr);

    //
    // Getters
    //

    std::string appDirectory() const;

    bool hasPlugin(const std::string& name) const;
    bool hasPlugin(const std::string& name, const std::string& minVersion) const;

    bool isPluginLoaded(const std::string& name) const;

    template<typename PluginType = IPlugin>
    std::shared_ptr<PluginType> pluginObject(const std::string& name);

    PluginInfo pluginInfo(const std::string& name) const;

private:

    struct PlugMgrPrivate;
    PlugMgrPrivate* const _p;
};

} // namespace jp

#endif // PLUGINMANAGER_H
