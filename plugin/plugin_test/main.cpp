#include <iostream>

#include <string>
#include "iplugin.h"
#include "whereami/src/whereami.h"

class PluginTest: public jp::IPlugin
{
    JP_DECLARE_PLUGIN(PluginTest, plugin_test)

public:

    std::string exePath()
    {
        const int length = wai_getExecutablePath(nullptr, 0, nullptr);
        if(length == -1)
            return std::string();
        char* path = (char*)malloc(length+1);
        if(wai_getExecutablePath(path, length, nullptr) != length)
            return std::string();

        path[length] = '\0';
        return path;
    }
    std::string modulePath()
    {
        const int length = wai_getModulePath(nullptr, 0, nullptr);
        if(length == -1)
            return std::string();
        char* path = (char*)malloc(length+1);
        if(wai_getModulePath(path, length, nullptr) != length)
            return std::string();

        path[length] = '\0';
        return path;
    }

    virtual void loaded()
    {
        std::cout << "Loading PluginTest" << std::endl;
        std::cout << "Exe: " << exePath() << std::endl;
        std::cout << "Module: " << modulePath() << std::endl;
        sendRequest(0);
    }

    virtual void aboutToBeUnloaded()
    {
        std::cout << "Unloading PluginTest" << std::endl;
    }

    ~PluginTest()
    {
        std::cout << "Destructing PluginTest" << std::endl;
    }

    virtual int handleRequest(int, const char *, void *, int)
    { return 0; }
};

JP_REGISTER_PLUGIN(PluginTest)
// This file is generated at configure time by CMake and contains meta.json data
// Must be placed AFTER JP_REGISTER_PLUGIN !!!
#include "metadata.h"
