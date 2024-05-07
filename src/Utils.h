#pragma once

#include <fstream>
#include <memory>
#include <sstream>
#include <llvm/IR/DebugInfoMetadata.h>

template <typename T>
using Own = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr Own<T> MakeOwn(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using Ref = std::shared_ptr<T>;
template <typename T, typename... Args>
constexpr Ref<T> MakeRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename Base>
constexpr Ref<T> StaticRefCast(const Ref<Base>& base)
{
    return std::static_pointer_cast<T>(base);
}

struct FileInfo
{
    std::string filename;
    std::string content;
    llvm::DIFile* debugFile;
};

struct Location
{
    uint32_t fileID;
    uint32_t line;
    uint32_t column;

    constexpr Location()
    {
        fileID = UINT32_MAX;
        line = 0;
        column = 0;
    }

    Location(uint32_t fileID, size_t index);

    FileInfo GetFile() const;
};

std::string ReadFile(const std::string& filename);
