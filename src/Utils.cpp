#include <Context.h>
#include <Utils.h>

std::string ReadFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
    {
        g_context->Error({ UINT32_MAX, 0 }, "Can't open file: %s", filename.c_str());
    }
    std::stringstream fileBuffer;
    fileBuffer << ifs.rdbuf();
    return fileBuffer.str();
}
