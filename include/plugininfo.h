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

#ifndef PLUGININFO_H
#define PLUGININFO_H

#include <cstdlib>

namespace jp
{

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Dependency
 * @brief Reprensents a dependency as specified in the meta.json file.
 *
 * A list of dependencies is stored in each PluginInfo object.
 * @see jp::PluginInfo
 */
struct Dependency
{
    const char* name; //!< The name of the dependency
    const char* version; //!< The version of the dependency
};

/**
 * @struct PluginInfo
 * @brief Struct that contains all plugin metadata.
 *
 * If name is an empty string, the metadata is invalid.
 */
struct PluginInfo
{
    const char* name; //!< The name of the plugin
    const char* prettyName; //!< The formatted name of the plugin (using for user outputs)
    const char* version; //!< The version of the plugin
    const char* author; //!< The author of the plugin
    const char* url; //!< The url of the plugin's website
    const char* license; //!< The license of the plugin
    const char* copyright; //!< The copyright statement of the plugin

    // Dependencies array
    int dependenciesNb = 0; //!< The number of dependencies
    Dependency* dependencies = nullptr; //!< The list of all dependencies

    /**
     * @brief Free all strings stored by this object
     */
    void free()
    {
        std::free((char*)name);
        std::free((char*)prettyName);
        std::free((char*)version);
        std::free((char*)author);
        std::free((char*)url);
        std::free((char*)license);
        std::free((char*)copyright);
        std::free((char*)dependencies);

        for(int i=0; i<dependenciesNb; ++i)
        {
            std::free((char*)dependencies[i].name);
            std::free((char*)dependencies[i].version);
        }
    }
};

#ifdef __cplusplus
} // extern "C"
#endif

} // namespace jp

#endif // PLUGININFO_H
