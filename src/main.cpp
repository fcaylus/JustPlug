#include <iostream>

#include "pluginmanager.h"

using namespace jp;

int main()
{
    PluginManager mgr;
    std::string appDir(mgr.appDirectory());
    mgr.searchForPlugins(appDir + "/plugin");
    mgr.loadPlugins();
    mgr.unloadPlugins();
}
