#pragma once

#include <forward_list>
#include <filesystem>

#define COMMAND_LIB_PATH       "/usr/lib64/bmc_clid/generator"
#define GENERATOR_LIB_PATH     "/usr/lib64/bmc_clid/plugin"

struct PluginProvider
{
public:
    void *addr;
    std::string name;

    PluginProvider() = delete;
    PluginProvider(const PluginProvider&) = delete;
    PluginProvider& operator=(const PluginProvider&) = delete;
    PluginProvider(PluginProvider &&) = delete;
    PluginProvider& operator=(PluginProvider&&) = delete;
    explicit PluginProvider(const char *fname);
    ~PluginProvider();
    bool isOpen() const;
};

std::forward_list<PluginProvider> loadProviders(const std::filesystem::path &libsPath);