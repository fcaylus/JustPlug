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

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <cstring> // for strcmp
#include <cstdint> // for intN_t types
#include "confinfo.h"

/*****************************************************************************/
/***** Macros definitions ****************************************************/
/*****************************************************************************/

/**
 * @brief Allow the plugin class to create the factory method.
 *
 * @a pluginName must be an ASCII string with only letters, digits and '_', and not starting with a digit
 * (just like a C identifier).
 * @note Must be declared AT THE BEGINNING of the class definition.
 * @related jp::IPlugin
 */
#define JP_DECLARE_PLUGIN(className, pluginName) JP_DECLARE_PLUGIN_CUSTOMPARENT(className, pluginName, jp::IPlugin)

/**
 * @brief Same as JP_DECLARE_PLUGIN, but allow to specify a custom parent
 *
 * Usefull when the parent class is an another interface that inherits itself from IPlugin.
 * Allows the user to create intermediate interfaces for different plugins.
 * @note Must be declared AT THE BEGINNING of the class definition.
 * @sa JP_DECLARE_PLUGIN
 * @related jp::IPlugin
 */
#define JP_DECLARE_PLUGIN_CUSTOMPARENT(className, pluginName, parentClass) _JP_DECLARE_PLUGIN__IMPL(className, pluginName, parentClass)

/**
 * @brief Allow to create intermediate interfaces for plugin (like IPlugin)
 *
 * When used, the child plugin should use JP_DECLARE_PLUGIN_CUSTOMPARENT and specify the interface created
 * with this macro as the parentClass
 * @note Must be declared AT THE BEGINNING of the class definition.
 * @sa JP_DECLARE_PLUGIN_CUSTOMPARENT
 * @related jp::IPlugin
 */
#define JP_DECLARE_INTERFACE(className, parentClass) _JP_DECLARE_INTERFACE__IMPL(className, parentClass)

/**
 * @brief Allow the plugin class to export the correct symbols.
 *
 * @note Must be declared AFTER the class definition.
 * @related jp::IPlugin
 */
#define JP_REGISTER_PLUGIN(className) _JP_REGISTER_PLUGIN__IMPL(className)

// Simply avoid the "unused" warning
#define JP_UNUSED(x) (void)x

/*****************************************************************************/
/***** IPlugin class *********************************************************/
/*****************************************************************************/

namespace jp
{

/**
 * @class IPlugin
 * @brief Base class for all plugins
 */
class IPlugin
{
public:
    virtual ~IPlugin() {}

    /**
     * @brief Called by the Plugin Manager when the plugin is loaded.
     *
     * The plugin class should use this function to do its initialisation stuff.
     * @note This function is always called after all dependencies have beeen loaded,
     * so it's safe to use them in this function.
     */
    virtual void loaded() = 0;
    /**
     * @brief Called by the Plugin Manager just before the unloading of the plugin.
     *
     * The plugin should use this function to do all his destruction stuff.
     * @note All dependencies remains valid until the return of this function.
     * @note The plugin object is deleted and the library unloaded just after the
     * return of this function.
     */
    virtual void aboutToBeUnloaded() = 0;

    /**
     * @brief Called by the Plugin Manager only if the plugin was registered as the main plugin.
     *
     * @note Always called after every plugins loaded() function.
     */
    virtual void mainPluginExec() {}

    /**
     * @brief Send a request to the plugin manager or other plugins
     * @param receiver The name of the receiver plugin (If NULL, the request is send to the plugin's manager)
     * @param code The code identifying the request. Each plugin has to provide a list of available codes
     * @param data A pointer to potential data to send (resp. retrieve) to (resp. from) the receiver.
     * @param dataSize A pointer to the size of the data. Must not be NULL if data are sent or expected.
     * @return A code depending on the success of the operation (0 on success). Each receiver has to describe to meaning of each code.
     *         Codes below 100 are reserved to RequestReturnCode enum.
     */
    virtual uint16_t sendRequest(const char* receiver,
                                 uint16_t code,
                                 void** data,
                                 uint32_t* dataSize) final
    {
        return sendRequestImpl(receiver, code, data, dataSize);
    }

    /**
     * @brief Handle request send by other plugins to this plugin.
     *
     * Each plugin have to re-implement this function if they want to be able to accept request from others.
     * If the plugin doesn't re-implement this function, it will return UNKNOWN_REQUEST code
     * by default.
     * @param sender
     * @param code
     * @param data
     * @param dataSize
     * @return
     */
    virtual uint16_t handleRequest(const char* sender,
                                   uint16_t code,
                                   void** data,
                                   uint32_t* dataSize)
    {
        JP_UNUSED(sender);JP_UNUSED(code);JP_UNUSED(data);JP_UNUSED(dataSize);
        return RequestReturnCode::UNKNOWN_REQUEST;
    }

    /**
     * @brief The ManagerRequest enum
     */
    enum ManagerRequest
    {
        // Get the app directory
        GET_APPDIRECTORY = 0,

        // Get the plugin API
        GET_PLUGINAPI = 1,
        // Get the number of plugins the manager is aware of
        GET_PLUGINSCOUNT = 2,

        // Get the PluginInfo object for the specified plugin (this plugin if data is null)
        GET_PLUGININFO = 10,
        // Get the version for the specified plugin (this plugin if data is null)
        GET_PLUGINVERSION = 11,

        // Check if the specified plugin exists
        CHECK_PLUGIN = 100,
        // Check if the specified plugin is loaded
        CHECK_PLUGINLOADED = 101,
    };

    /**
     * @brief The RequestReturnCode enum
     */
    enum RequestReturnCode
    {
        SUCCESS = 0,
        COMMON_ERROR = 1,
        UNKNOWN_REQUEST = 2,
        DATASIZE_NULL = 3,
        NOT_A_DEPENDENCY = 4,

        // Used for CHECK_* requests
        RESULT_TRUE = SUCCESS,
        RESULT_FALSE = COMMON_ERROR,

        NOT_FOUND = 5,

        USER_RETURN_CODE = 100
    };

protected:

// Implementation macro used to define the function pointer with the correct type
// Typedefs are not used because the signature is also used by PluginManager and it
// should not be part of the public API of IPlugin.

#define _JP_MGR_REQUEST_FUNC_SIGNATURE(varName) \
    uint16_t (*varName)(const char*, uint16_t, void**, uint32_t*)

    //! @cond
    // Constructor used by the factory method to creates the plugin object
    IPlugin(_JP_MGR_REQUEST_FUNC_SIGNATURE(requestFunc),
            IPlugin** depPlugins,
            int depNb)
        : _requestFunc(requestFunc),
          _depPlugins(depPlugins),
          _depNb(depNb)
    {}

    //! @endcond

private:
    _JP_MGR_REQUEST_FUNC_SIGNATURE(_requestFunc);
    IPlugin** _depPlugins;
    int _depNb;

    virtual const char* jp_name() = 0;

    IPlugin() = default;

    // Prevent from copying plugins objects
    IPlugin(const IPlugin&) = delete;
    const IPlugin& operator=(const IPlugin&) = delete;

    // Private implementation of sendRequest
    uint16_t sendRequestImpl(const char *receiver, uint16_t code, void **data, uint32_t *dataSize)
    {
        // Send to manager (receiver is null)
        if(!receiver)
            return _requestFunc(jp_name(), code, data, dataSize);
        // Send to the dependency
        for(int i=0; i < _depNb; ++i)
        {
            if(strcmp(receiver, _depPlugins[i]->jp_name()) == 0)
                return _depPlugins[i]->handleRequest(jp_name(), code, data, dataSize);
        }

        // Dependency was not found
        return IPlugin::NOT_A_DEPENDENCY;
    }
};

} // namespace jp


/*****************************************************************************/
/***** MACRO IMPLEMENTATION **************************************************/
/*****************************************************************************/

/* Defines the correct export symbol */

// Checks for GCC compiler
#ifdef CONFINFO_COMPILER_GCC
#  if __GNUC__ >= 4
#    if defined(CONFINFO_PLATFORM_WIN32) && !defined(CONFINFO_PLATFORM_CYGWIN)
#      define JP_EXPORT_SYMBOL __attribute__((__dllexport__))
#    else
#      define JP_EXPORT_SYMBOL __attribute__((__visibility__("default")))
#    endif
#  else
#    define JP_EXPORT_SYMBOL
#  endif
#endif

// Checks for SUNPRO_CC compiler
#if defined(CONFINFO_COMPILER_SUNPRO_CC) && __SUNPRO_CC > 0x500
#  define JP_EXPORT_SYMBOL __global
#endif

// Checks for CLang and IBM compiler on other platforms than Windows
#if (defined(CONFINFO_COMPILER_CLANG) || defined(CONFINFO_COMPILER_XLCPP)) && !defined(CONFINFO_PLATFORM_WIN32)
#  define JP_EXPORT_SYMBOL __attribute__((__visibility__("default")))
#endif

// Checks for intel compiler
#if defined(CONFINFO_COMPILER_INTEL) && defined(__GNUC__) && (__GNUC__ >= 4)
#  define JP_EXPORT_SYMBOL __attribute__((__visibility__("default")))
#endif

// Default export symbol on Windows and others
#ifndef JP_EXPORT_SYMBOL
#  ifdef CONFINFO_PLATFORM_WIN32
#    define JP_EXPORT_SYMBOL __declspec(dllexport)
#  else
#    define JP_EXPORT_SYMBOL
#  endif
#endif

/* Functions used for checks in different macros */
namespace jp_private
{
namespace CStringUtil
{
// Returns true if str contains c
constexpr inline bool contains(const char* str, const char c)
{
    return (*str == 0) ? false : (str[0] == c ? true: contains(++str, c));
}
// Returns true if str contains only characters from 'allowed'
constexpr inline bool containsOnly(const char* str, const char* allowed)
{
    return (*str == 0) ? true : (contains(allowed, str[0]) ? containsOnly(++str, allowed) : false);
}
} // namespace CStringUtil
} // namespace jp_private

#define _JP_DECLARE_INTERFACE__IMPL(className, parentClass)                                         \
    protected:                                                                                      \
        className(_JP_MGR_REQUEST_FUNC_SIGNATURE(requestFunc),                                      \
                  jp::IPlugin** depPlugins,                                                         \
                  int depNb)                                                                        \
            : parentClass(requestFunc, depPlugins, depNb) {}

#define _JP_DECLARE_PLUGIN__IMPL(className, pluginName, parentClass)                                \
    static_assert(jp_private::CStringUtil::containsOnly(#pluginName,                                \
                                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                \
                                                        "abcdefghijklmnopqrstuvwxyz0123456789_"),   \
                  "Plugin name \"" #pluginName "\" must contains only letters, digits and '_'");    \
    static_assert(!jp_private::CStringUtil::contains("0123456789", *(const char*)(#pluginName)),    \
                  "Plugin name \"" #pluginName "\" cannot start with a digit");                     \
    static_assert(*(const char*)(#pluginName) != '\0',                                              \
                  "Plugin name must not be an empty string !");                                     \
    _JP_DECLARE_INTERFACE__IMPL(className, parentClass)                                             \
    protected:                                                                                      \
        const char* jp_name() override { return #pluginName; }                                      \
    public:                                                                                         \
        static jp::IPlugin* jp_createPlugin(_JP_MGR_REQUEST_FUNC_SIGNATURE(requestFunc),            \
                                            jp::IPlugin** depPlugins,                               \
                                            int depNb)                                              \
        {                                                                                           \
            className* ptr = new className(requestFunc, depPlugins, depNb);                         \
            return ptr;                                                                             \
        }                                                                                           \
        static constexpr const char* name() { return #pluginName; }


#define _JP_EXPORT_FUNCTION_ALIAS(name, alias)                                              \
    extern "C" JP_EXPORT_SYMBOL const void* alias;                                          \
    const void* alias = reinterpret_cast<const void*>(reinterpret_cast<intptr_t>(&name));


#define _JP_REGISTER_PLUGIN__IMPL(className)                                \
    extern "C" JP_EXPORT_SYMBOL const char* jp_name;                        \
    const char* jp_name = className::name();                                \
    extern "C" JP_EXPORT_SYMBOL const char jp_metadata[];                   \
    _JP_EXPORT_FUNCTION_ALIAS(className::jp_createPlugin, jp_createPlugin)

#endif // IPLUGIN_H
