#pragma once

#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
static_assert(false, "Unsupported platform");
#endif

namespace cask {

inline std::string executable_directory() {
#if defined(__APPLE__)
    uint32_t buffer_size = 1024;
    char buffer[1024];
    _NSGetExecutablePath(buffer, &buffer_size);
    std::string full_path(buffer);
    auto last_separator = full_path.rfind('/');
    return full_path.substr(0, last_separator + 1);
#elif defined(__linux__)
    char buffer[1024];
    ssize_t bytes_read = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    std::string full_path(buffer);
    auto last_separator = full_path.rfind('/');
    return full_path.substr(0, last_separator + 1);
#elif defined(_WIN32)
    char buffer[1024];
    GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    std::string full_path(buffer);
    auto last_separator = full_path.rfind('\\');
    return full_path.substr(0, last_separator + 1);
#endif
}

}
