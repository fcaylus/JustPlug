#include <iostream>

#include "iplugin.h"

class Plugin: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(Plugin)

public:

    virtual void loaded()
    {
        std::cout << "Loading Plugin 3" << std::endl;
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading Plugin 3" << std::endl;
    }
};

JP_REGISTER_PLUGIN(Plugin, plugin_3)
#include "metadata.h"
