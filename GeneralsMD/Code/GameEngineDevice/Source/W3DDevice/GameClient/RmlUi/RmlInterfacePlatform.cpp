#include "W3DDevice/GameClient/RmlUi/RmlInterfacePlatform.h"

#include "Common/GlobalData.h"
#include "Common/FileSystem.h"
#include "Common/file.h"
#include <windows.h>
#include <commdlg.h>
#include <stb_image.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

namespace RmlInterfacePlatform
{
namespace fs = std::filesystem;

static std::string utf8(const fs::path &path)
{
    const auto text = path.generic_u8string();
    return std::string(reinterpret_cast<const char *>(text.data()), text.size());
}

bool readFile(const std::string &path, std::vector<unsigned char> &bytes)
{
    bytes.clear();
    try
    {
        std::ifstream stream(fs::u8path(path), std::ios::binary);
        if (stream)
        {
            bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
            return true;
        }
    }
    catch (const std::exception &) {}
    File *file = TheFileSystem ? TheFileSystem->openFile(path.c_str(), File::READ | File::BINARY) : nullptr;
    if (!file) return false;
    const int size = file->size();
    if (size > 0) { bytes.resize(size); file->read(bytes.data(), size); }
    file->close();
    return true;
}

std::string extension(const std::string &path)
{
    std::string ext;
    try { ext = utf8(fs::u8path(path).extension()); }
    catch (const std::exception &) { return {}; }
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return ext;
}

std::vector<std::string> catalog(const char *folder, bool images)
{
    std::set<std::string> paths;
    std::vector<fs::path> folders = {fs::u8path(std::string("UI/") + folder)};
    if (TheGlobalData)
    {

        const std::string narrow = std::string(TheGlobalData->getPath_UserData().str()) + "UI/" + folder;
        const int length = MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, nullptr, 0);
        if (length > 0)
        {
            std::wstring wide(length, L'\0');
            MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, wide.data(), length);
            wide.resize(length - 1);
            folders.emplace_back(wide);
        }
    }
    for (const auto &directory : folders)
    {
        std::error_code ec;
        fs::directory_iterator iterator(directory, ec), end;
        while (!ec && iterator != end)
        {
            if (iterator->is_regular_file(ec))
            {
                const std::string path = utf8(iterator->path());
                const std::string ext = extension(path);
                if ((!images && ext == ".rcss") || (images && (ext == ".png" || ext == ".jpg" || ext == ".jpeg")))
                    paths.insert(utf8(fs::absolute(iterator->path(), ec)));
            }
            iterator.increment(ec);
        }
    }
    return {paths.begin(), paths.end()};
}

std::string displayName(const std::string &path)
{
    try { return utf8(fs::u8path(path).filename()); }
    catch (const std::exception &) { return path; }
}
std::string browseBackground()
{
    wchar_t path[32768] = {};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = L"Background images (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = DWORD(std::size(path));
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    return GetOpenFileNameW(&dialog) ? utf8(fs::path(path)) : std::string();
}

bool imageSize(const std::string &path, int &width, int &height)
{
    width = height = 0;
    int channels = 0;
    Bytes bytes;
    return readFile(path, bytes) && !bytes.empty() &&
        stbi_info_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels) &&
        width > 0 && height > 0;
}
}
