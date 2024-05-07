#include <Context.h>
#include <Utils.h>

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

std::string ReadFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
    {
        g_context->Error({}, "Can't open file: %s", filename.c_str());
    }
    std::stringstream fileBuffer;
    fileBuffer << ifs.rdbuf();
    return fileBuffer.str();
}
