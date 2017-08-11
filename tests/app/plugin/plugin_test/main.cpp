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

#include <iostream>

#include <string>
#include "iplugin.h"

class PluginTest: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(PluginTest, plugin_test)

public:

    void loaded() override
    {
        std::cout << "Loading PluginTest" << std::endl;
        //sendRequest(0);
    }

    void aboutToBeUnloaded() override
    {
        std::cout << "Unloading PluginTest" << std::endl;
    }

    ~PluginTest()
    {
        std::cout << "Destructing PluginTest" << std::endl;
    }

    uint16_t handleRequest(const char*, uint16_t, void*, uint32_t*) override
    { return 0; }
};

JP_REGISTER_PLUGIN(PluginTest)
// This file is generated at configure time by CMake and contains meta.json data
// Must be placed AFTER JP_REGISTER_PLUGIN !!!
#include "metadata.h"
