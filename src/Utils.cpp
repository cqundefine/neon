#include <Context.h>
#include <Utils.h>
#include <filesystem>
#include <fstream>

Location::Location(uint32_t fileID, size_t index)
{
    this->fileID = fileID;
    auto [line, column] = g_context->LineColumnFromLocation(fileID, index);
    this->line = line;
    this->column = column;
}

FileInfo Location::GetFile() const
{
    return g_context->files.at(fileID);
}

std::vector<char> ReadFile(const std::filesystem::path& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        g_context->Error({}, "Can't open file: %s", filename.string().c_str());

    auto length { std::filesystem::file_size(filename) };
    std::vector<char> result(length);
    file.read(reinterpret_cast<char*>(result.data()), static_cast<long>(length));

    return result;
}
