#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <memory> // for std::shared_ptr

#include "confinfo.h"

/* Functions used for checks in different macros */
namespace jp {
namespace CStringUtil {
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
} // namespace jp

/*****************************************************************************/
/***** Macros definitions ****************************************************/
/*****************************************************************************/

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

/**
 * @brief Allow the plugin class to create the factory method.
 * @note Must be declared AT THE BEGINNING of the class definition.
 */
#define JP_DECLARE_PLUGIN(className)                                \
    public:                                                         \
        static std::shared_ptr<className> jp_createPlugin()         \
        {                                                           \
            return std::shared_ptr<className>(new className());     \
        }

/**
 * @brief Allow the plugin class to export the correct symbols.
 * @a pluginName must be an ASCII string with only letters, digits and '_', and not starting with a digit
 * (just like a C identifier).
 *
 * @note Must be declared AFTER the class definition.
 */
#define JP_REGISTER_PLUGIN(className, pluginName)                                                                   \
    static_assert(jp::CStringUtil::containsOnly(#pluginName,                                                        \
                                                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"), \
                  "Plugin name \"" #pluginName "\" must contains only letters, digits and '_'");                    \
    static_assert(!jp::CStringUtil::contains("0123456789", *(const char*)(#pluginName)),                            \
                  "Plugin name \"" #pluginName "\" cannot start with a digit");                                     \
    extern "C" JP_EXPORT_SYMBOL const char* jp_name;                                                                \
    const char* jp_name = #pluginName;                                                                              \
    extern "C" JP_EXPORT_SYMBOL const char jp_metadata[];                                                           \
    extern "C" JP_EXPORT_SYMBOL const void *jp_createPlugin;                                                        \
    const void * jp_createPlugin = reinterpret_cast<const void*>(                                                   \
                                   reinterpret_cast<intptr_t>(&className::jp_createPlugin));


/*****************************************************************************/
/***** IPlugin class *********************************************************/
/*****************************************************************************/

namespace jp {

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
     * The plugin class should use this function to do it's initialisation stuff.
     * @note This function is always called after all dependencies have beeen loaded,
     * so it's safe to use them in this function.
     */
    virtual void loaded() = 0;
    /**
     * @brief Called by the Plugin Manager just before the unloading of the plugin.
     * The plugin should use this function to do all his destriction stuff.
     * @note All dependencies remains valid until the return of this function.
     * @note The plugin object is deleted and the library unloaded just after the
     * return of this function.
     */
    virtual void aboutToBeUnloaded() = 0;

protected:
    IPlugin() {}
};

} // namespace jp

#endif // IPLUGIN_H
