#include "platform.hpp"
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
std::filesystem::path executable_directory() {
    std::vector<wchar_t> buffer(32768);
    DWORD n = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
    if (!n || n >= buffer.size())
        throw std::runtime_error("Cannot find executable directory");
    return std::filesystem::path(std::wstring(buffer.data(), n)).parent_path();
}
#else
#include <unistd.h>
std::filesystem::path executable_directory() {
    std::vector<char> buffer(4096);
    for (;;) {
        auto n = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (n < 0)
            throw std::runtime_error("Cannot read /proc/self/exe");
        if (size_t(n) < buffer.size())
            return std::filesystem::path(std::string(buffer.data(), size_t(n))).parent_path();
        buffer.resize(buffer.size() * 2);
    }
}
#endif
