#ifndef PLUGININFO_H
#define PLUGININFO_H

namespace jp
{

/**
 * @struct Dependency
 * @brief Reprensents a dependency as specified in the meta.json file.
 */
struct Dependency
{
    const char* name;
    const char* version;

    ~Dependency();
};

/**
 * @struct PluginInfo
 * @brief Struct that contains all plugin metadata.
 * If name is an empty string, the metadata is invalid.
 */
struct PluginInfo
{
    const char* name;
    const char* prettyName;
    const char* version;
    const char* author;
    const char* url;
    const char* license;
    const char* copyright;

    // Dependencies array
    int dependenciesNb = 0;
    Dependency* dependencies = nullptr;

    /**
     * @brief Convert the struct to a printable string.
     * The user must free the string himself.
     * @return the string
     * @note This function may be slow due to the internal conversion to std::string
     */
    const char* toString() const;

    ~PluginInfo();
};

} // namespace jp

#endif // PLUGININFO_H
