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

#ifndef PLUGIN_H
#define PLUGIN_H

/*
 * This file is an internal header. It's not part of the public API,
 * and may change at any moment.
 */

#include <string> // for std::string
#include <memory> // for std::shared_ptr
#include <vector> // for std::vector
#include <functional> // for std::function

#include "plugininfo.h"
#include "iplugin.h"
#include "sharedlibrary.h"

#include "tribool.h"

namespace jp_private
{

// PluginInfoStd is used internally by the PLuginManager
// When the user wants to access the metadata, a PluginInfo object is returned
// with only C-String to ensure ABI compatibility
struct PluginInfoStd
{
    std::string name;
    std::string prettyName;
    std::string version;
    std::string author;
    std::string url;
    std::string license;
    std::string copyright;

    struct Dependency
    {
        std::string name;
        std::string version;
    };

    std::vector<Dependency> dependencies;

    // A copy of each string is performed
    jp::PluginInfo toPluginInfo();

    std::string toString();
};

// Internal structure to store plugins and their associated library
struct Plugin
{
    typedef jp::IPlugin* (iplugin_create_t)(_JP_MGR_REQUEST_FUNC_SIGNATURE(),
                                            jp::IPlugin**,
                                            int);

    std::shared_ptr<jp::IPlugin> iplugin;
    std::function<iplugin_create_t> creator;
    jp::SharedLibrary lib;

    std::string path;
    PluginInfoStd info;

    //
    // Flags used when loading

    // true if all dependencies are present, indeterminate if not yet checked
    TriBool dependenciesExists = TriBool::Indeterminate;
    int graphId = -1;

    // Destructor
    virtual ~Plugin();

    Plugin() = default;

    // Non-copyable
    Plugin(const Plugin&) = delete;
    const Plugin& operator=(const Plugin&) = delete;
};
typedef std::shared_ptr<Plugin> PluginPtr;

} // namespace jp_private

#endif // PLUGIN_H
