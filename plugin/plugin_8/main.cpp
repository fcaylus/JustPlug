#include <iostream>

#include "iplugin.h"

class Plugin: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(Plugin)

public:

    virtual void loaded()
    {
        std::cout << "Loading Plugin 8" << std::endl;
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading Plugin 8" << std::endl;
    }
};

JP_REGISTER_PLUGIN(Plugin, plugin_8)
#include "metadata.h"
