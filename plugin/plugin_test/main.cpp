#include <iostream>

#include "iplugin.h"

class PluginTest: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(PluginTest)

private:
    PluginTest()
    {
        std::cout << "Constructing PluginTest" << std::endl;
    }

public:

    virtual void loaded()
    {
        std::cout << "Loading PluginTest" << std::endl;
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading PluginTest" << std::endl;
    }

    ~PluginTest()
    {
        std::cout << "Destructing PluginTest" << std::endl;
    }
};

JP_REGISTER_PLUGIN(PluginTest, plugin_test)
// This file is generated at configure time by CMake and contains meta.json data
// Must be placed AFTER JP_REGISTER_PLUGIN !!!
#include "metadata.h"
