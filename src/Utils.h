#pragma once

#include <filesystem>
#include <llvm/IR/DebugInfoMetadata.h>
#include <memory>

#ifdef ALWAYS_INLINE
    #undef ALWAYS_INLINE
#endif
#define ALWAYS_INLINE __attribute__((always_inline)) inline

template <typename From, typename To>
using CopyConst = std::conditional_t<std::is_const_v<From>, std::add_const_t<To>, std::remove_const_t<To>>;

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
    std::vector<char> content;
    llvm::DIFile* debugFile;
};

struct Location
{
    std::optional<uint32_t> fileID;
    uint32_t line;
    uint32_t column;

    constexpr Location()
    {
        fileID = {};
        line = 0;
        column = 0;
    }

    Location(uint32_t fileID, size_t index);

    FileInfo GetFile() const;
};

std::vector<char> ReadFile(const std::filesystem::path& filename);
