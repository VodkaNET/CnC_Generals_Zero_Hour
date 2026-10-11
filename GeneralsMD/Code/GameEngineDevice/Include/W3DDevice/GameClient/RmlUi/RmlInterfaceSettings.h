
#pragma once

#include "W3DDevice/GameClient/RmlUi/RmlInterfacePlatform.h"

namespace Rml { class Context; }

namespace RmlInterfaceSettings
{
struct Values
{
    int scalePercent = 100;
    int controlBarScalePercent = 100;
    bool darkTheme = false;
    std::string customTheme;
    bool enableBackground = false;
    std::string backgroundImage;
    bool hideBorder = false;
};

const Values &current();
std::string stylePath(const std::string &path);
bool save(const Values &values, std::string &error);
std::vector<std::string> themeFiles();
std::vector<std::string> backgroundFiles();
std::string browseBackground();
std::string displayName(const std::string &path);
bool readBackground(std::vector<unsigned char> &bytes);
void apply(Rml::Context *context, int width, int height);
void reset();
}
