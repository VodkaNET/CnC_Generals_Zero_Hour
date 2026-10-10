#pragma once

#include <string>
#include <vector>

namespace RmlInterfacePlatform
{
using Paths = std::vector<std::string>;
using Bytes = std::vector<unsigned char>;

bool readFile(const std::string &path, Bytes &bytes);
std::string extension(const std::string &path);
Paths catalog(const char *folder, bool images);
std::string displayName(const std::string &path);
std::string browseBackground();
bool imageSize(const std::string &path, int &width, int &height);
}
