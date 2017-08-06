#include <iostream>

#include "pluginmanager.h"

using namespace jp;

void callBackFunc(const ReturnCode& code, const char* data)
{
    std::cout << code.message();
    if(data)
        std::cout << " (" << data << ")";
    std::cout << std::endl;
}

int main()
{
    PluginManager& mgr = PluginManager::instance();
    std::string appDir(mgr.appDirectory());
    std::cout << appDir << std::endl;
    mgr.searchForPlugins(appDir + "/plugin", callBackFunc);
    mgr.loadPlugins(callBackFunc);
    mgr.unloadPlugins(callBackFunc);
}
