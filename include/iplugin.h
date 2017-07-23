#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <memory> // for std::shared_ptr

#include <boost/dll/alias.hpp>

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

// Allow the child class to create the factory method
// Must be declared at the beginning of the class definition
#define JP_DECLARE_PLUGIN(className)                                \
    public:                                                         \
        static std::shared_ptr<className> jp_createPlugin()         \
        {                                                           \
            return std::shared_ptr<className>(new className());     \
        }

// Allow the child class to export the correct symbol
// Must be declared AFTER the class definition
// pluginName must be a ASCII string with only letters, digits and '_', and not starting with a digit
// (just like a C++ identifier)
#define JP_REGISTER_PLUGIN(className, pluginName)                                                                               \
    static_assert(CStringUtil::containsOnly(#pluginName, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"),    \
                  "Plugin name \"" #pluginName "\" must contains only letters, digits and '_'");                                \
    static_assert(!CStringUtil::contains("0123456789", *(const char*)(#pluginName)),                                            \
                  "Plugin name \"" #pluginName "\" cannot start with a digit");                                                 \
    extern "C" BOOST_SYMBOL_EXPORT const char* jp_name;                                                                         \
    const char* jp_name = #pluginName;                                                                                          \
    BOOST_DLL_ALIAS(className::jp_createPlugin, jp_createPlugin)                                                                \
    extern "C" BOOST_SYMBOL_EXPORT const char jp_metadata[];


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
