#include "common/files.h"

#include <fstream>
#include <iterator>
#include <system_error>

namespace files {

std::optional<std::string> read(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) return std::nullopt;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return bytes;
}

bool write(const std::filesystem::path& file, std::string_view bytes)
{
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    std::filesystem::path temp = file;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) {
            out.close();
            std::filesystem::remove(temp, ec);
            return false;
        }
    }
    std::filesystem::rename(temp, file, ec);
    if (ec) {
        // Some C libraries on Windows don't rename over an existing file.
        std::filesystem::remove(file, ec);
        std::filesystem::rename(temp, file, ec);
    }
    if (ec) std::filesystem::remove(temp, ec);
    return !ec;
}

} // namespace files
