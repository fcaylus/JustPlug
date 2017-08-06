#include "plugininfo.h"

#include <string>
#include <cstring> // for strdup

using namespace jp;
using namespace std;

const char * PluginInfo::toString() const
{
    string str = "Plugin info:\n";
    str += "Name: " + string(name) + "\n";
    str += "Pretty name: " + string(prettyName) + "\n";
    str += "Version: " + string(version) + "\n";
    str += "Author: " + string(author) + "\n";
    str += "Url: " + string(url) + "\n";
    str += "License: " + string(license) + "\n";
    str += "Copyright: " + string(copyright) + "\n";
    str += "Dependencies:\n";
    for(int i=0; i < dependenciesNb; ++i)
        str += " - " + string(dependencies[i].name) + " (" + string(dependencies[i].version) + ")\n";
    return strdup(str.c_str());
}

Dependency::~Dependency()
{
    //delete name;
    //delete version;
}

PluginInfo::~PluginInfo()
{
    /*free((char*)name);
    free((char*)prettyName);
    free((char*)version);
    free((char*)author);
    free((char*)url);
    free((char*)license);
    free((char*)copyright);*/
    //delete dependencies;
}
