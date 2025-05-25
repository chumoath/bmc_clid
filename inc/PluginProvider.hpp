#pragma once

#include <forward_list>
#include <filesystem>

/*
 * 使用 static 或者 不加 inline，会有多个副本；符号类型为 d，表示初始化的全局/静态变量。
 * 使用 inline constexpr，符号类型为 u，表示唯一全局符号。
 */
inline constexpr const char *COMMAND_LIB_PATH = "/usr/lib/bmc_clid/generator";
inline constexpr const char *GENERATOR_LIB_PATH = "/usr/lib/bmc_clid/plugin";

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
