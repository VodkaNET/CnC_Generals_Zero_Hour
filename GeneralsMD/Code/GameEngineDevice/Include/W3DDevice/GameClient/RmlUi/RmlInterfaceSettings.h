
#pragma once

#include "W3DDevice/GameClient/RmlUi/RmlInterfacePlatform.h"

namespace Rml { class Context; }

namespace RmlInterfaceSettings
{
struct Values
{
    int scalePercent = 100;
    int controlBarScalePercent = 100;
    bool mobileMode = false;
    bool mobileUi = false;
    int mobileMinimapPercent = 100;
    bool darkTheme = false;
    std::string customTheme;
    bool enableBackground = false;
    std::string backgroundImage;
    bool hideBorder = false;
    bool showEnergyNumbers = false;
    bool showUnitName = false;
    bool showGeneralRank = false;
    bool showPromotionPoints = false;
    bool showIconHotkeys = true;
    bool colorDisabledIcons = false;
    int productionProgressStyle = 0;
};

const Values &current();
int effectiveScalePercent();
int effectiveControlBarScalePercent();
std::string stylePath(const std::string &path);
std::string scaleControlBarStyle(const std::string &source);
bool save(const Values &values, std::string &error);
std::vector<std::string> themeFiles();
std::vector<std::string> backgroundFiles();
std::string browseBackground();
std::string displayName(const std::string &path);
bool readBackground(std::vector<unsigned char> &bytes);
void apply(Rml::Context *context, int width, int height);
void reset();
}
