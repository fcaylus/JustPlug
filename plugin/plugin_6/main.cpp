#include <iostream>

#include "iplugin.h"

class Plugin: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(Plugin, plugin_6)

public:

    virtual void loaded()
    {
        std::cout << "Loading Plugin 6" << std::endl;
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading Plugin 6" << std::endl;
    }

    virtual int handleRequest(int, const char *, void *, int)
    { return 0; }
};

JP_REGISTER_PLUGIN(Plugin)
#include "metadata.h"
