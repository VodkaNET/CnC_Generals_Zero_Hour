#include "W3DDevice/GameClient/RmlUi/RmlLoadingTips.h"
#include "Common/FileSystem.h"
#include "Common/file.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "Common/PlayerList.h"
#include "Common/Player.h"
#include "Common/UnicodeUtf8.h"
#include "GameClient/ControlBar.h"
#include "GameClient/GameText.h"
#include "GameClient/Image.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <vector>

namespace RmlLoadingTips
{
namespace
{
struct Entry
{
    std::string faction, title, advice, author, command, object, upgrade;
};
std::vector<Entry> entries;
std::map<std::string, std::size_t> cursors;
bool loaded = false;
std::string trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
std::string factionFor(std::string side)
{
    for (char &c : side) c = char(std::tolower(static_cast<unsigned char>(c)));
    if (side.find("america") != std::string::npos || side == "usa") return "USA";
    if (side.find("gla") != std::string::npos) return "GLA";
    if (side.find("china") != std::string::npos) return "CHINA";
    return {};
}
void load()
{
    if (loaded || !TheFileSystem) return;
    File *file = TheFileSystem->openFile("UI/LoadingTips.ini", File::READ | File::BINARY);
    if (!file) return;
    std::string text(std::size_t(file->size()), '\0');
    if (!text.empty()) file->read(&text[0], Int(text.size()));
    file->close();
    std::istringstream input(text);
    std::string line;
    Entry *entry = nullptr;
    while (std::getline(input, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']')
        {
            entries.emplace_back();
            entry = &entries.back();
            continue;
        }
        const auto separator = line.find('=');
        if (!entry || separator == std::string::npos) continue;
        const auto key = trim(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));
        if (key == "Faction") entry->faction = value;
        else if (key == "Title") entry->title = value;
        else if (key == "Advice") entry->advice = value;
        else if (key == "Author") entry->author = value;
        else if (key == "Command") entry->command = value;
        else if (key == "Object") entry->object = value;
        else if (key == "Upgrade") entry->upgrade = value;
    }
    entries.erase(std::remove_if(entries.begin(), entries.end(), [](const Entry &entry) {
        return entry.title.empty() || entry.advice.empty();
    }), entries.end());
    loaded = true;
}
std::string gameText(const AsciiString &label)
{
    return TheGameText && !label.isEmpty() ? unicodeToUtf8(TheGameText->fetch(label)) : std::string();
}
}

std::size_t select(const std::string &side)
{
    load();
    if (entries.empty()) return std::string::npos;
    const auto faction = factionFor(side);
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (faction.empty() || entries[i].faction == faction) candidates.push_back(i);
    if (candidates.empty())
        for (std::size_t i = 0; i < entries.size(); ++i) candidates.push_back(i);
    return candidates[cursors[faction]++ % candidates.size()];
}

Tip resolve(std::size_t index, const std::string &side)
{
    load();
    if (index >= entries.size()) return {};
    const auto &entry = entries[index];
    Tip tip;
    tip.faction = entry.faction + " FACTION";
    tip.title = entry.title;
    tip.advice = entry.advice;
    tip.author = entry.author;
    tip.valid = true;
    const CommandButton *command = TheControlBar && !entry.command.empty()
        ? TheControlBar->findCommandButton(entry.command.c_str()) : nullptr;
    if (command)
    {
        tip.description = gameText(command->getDescriptionLabel());
        if (const Image *image = command->getButtonImage()) tip.image = image->getName().str();
    }
    Player *player = nullptr;
    if (ThePlayerList)
        for (Int i = 0; i < ThePlayerList->getPlayerCount(); ++i)
        {
            auto *candidate = ThePlayerList->getNthPlayer(i);
            if (candidate && candidate->getSide().str() == side) { player = candidate; break; }
        }
    Int cost = -1;
    const ThingTemplate *object = command ? command->getThingTemplate() : nullptr;
    if (!object && !entry.object.empty() && TheThingFactory)
        object = TheThingFactory->findTemplate(entry.object.c_str(), FALSE);
    const UpgradeTemplate *upgrade = command ? command->getUpgradeTemplate() : nullptr;
    if (!upgrade && !entry.upgrade.empty() && TheUpgradeCenter)
        upgrade = TheUpgradeCenter->findUpgrade(entry.upgrade.c_str());
    if (object)
    {
        cost = player ? object->calcCostToBuild(player) : object->friend_getBuildCost();
        if (!command) tip.title = unicodeToUtf8(object->getDisplayName());
    }
    else if (upgrade) cost = upgrade->calcCostToBuild(player);
    if (cost >= 0) tip.cost = "$" + std::to_string(cost);
    return tip;
}
}
