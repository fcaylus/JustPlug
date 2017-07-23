#include <iostream>

#include "iplugin.h"

class Plugin: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(Plugin)

public:

    virtual void loaded()
    {
        std::cout << "Loading Plugin 9" << std::endl;
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading Plugin 9" << std::endl;
    }
};

JP_REGISTER_PLUGIN(Plugin, plugin_9)
#include "metadata.h"
