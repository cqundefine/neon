#pragma once

#include <fstream>
#include <memory>
#include <sstream>

template<typename T>
using Own = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Own<T> MakeOwn(Args&& ... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> MakeRef(Args&& ... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}
template<typename T, typename Base>
constexpr Ref<T> StaticRefCast(const Ref<Base>& base)
{
    return std::static_pointer_cast<T>(base);
}

struct Location
{
    uint32_t fileID;
    size_t index;
};

inline std::string ReadFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    std::stringstream fileBuffer;
    fileBuffer << ifs.rdbuf();
    return fileBuffer.str();
}
