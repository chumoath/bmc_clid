#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <forward_list>
#include <dlfcn.h>
#include "PluginProvider.hpp"

constexpr const char PluginExtn[] = ".so";

PluginProvider::PluginProvider(const char *fname) : addr(nullptr), name(fname)
{
    std::cout << "Open Plugin provider library: " << name << std::endl;
    try {
        addr = dlopen(name.c_str(), RTLD_NOW);
    } catch (const std::exception &e) {
        std::cout << "ERROR opening Plugin provider " << name << ": " << e.what() << std::endl;
    }

    if ( !isOpen() ) {
        std::cout << "ERROR opening Plugin provider " << name << ": " << dlerror() << std::endl;
    }
}

PluginProvider::~PluginProvider()
{
    if ( isOpen() ) {
        dlclose(addr);
    }
}

bool PluginProvider::isOpen() const
{
    return (nullptr != addr);
}

std::forward_list<PluginProvider> loadProviders(const std::filesystem::path &libsPath)
{
    std::vector<std::filesystem::path> libs;
    for ( const auto &libPath: std::filesystem::directory_iterator(libsPath) ) {
        std::error_code ec;
        std::filesystem::path fname = libPath.path();
        if ( std::filesystem::is_symlink(fname, ec) || ec ) {
            // it's a symlink or some other error; skip it
            continue;
        }

        while ( fname.has_extension() ) {
            std::filesystem::path extn = fname.extension();
            if ( extn == PluginExtn ) {
                libs.push_back(libPath.path());
                break;
            }

            fname.replace_extension();
        }
    }

    std::sort(libs.begin(), libs.end());

    std::forward_list<PluginProvider> handles;
    for ( auto &lib: libs) {
        handles.emplace_front(lib.c_str());
    }

    return handles;
}