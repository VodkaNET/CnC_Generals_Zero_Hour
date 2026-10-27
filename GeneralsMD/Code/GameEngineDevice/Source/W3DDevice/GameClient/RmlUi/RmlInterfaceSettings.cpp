#include "W3DDevice/GameClient/RmlUi/RmlInterfaceSettings.h"
#include "W3DDevice/GameClient/RmlUi/RmlTextShaping.h"

#include "Common/GlobalData.h"
#include "Common/UserPreferences.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi/Core/Factory.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <regex>

namespace RmlInterfaceSettings
{
namespace
{
Values values;
bool loaded = false;
unsigned revision = 1;
const char *preferenceFile = "GeneralsOnlineInterface.ini";

struct DocumentState
{
    Rml::SharedPtr<Rml::StyleSheetContainer> base;
    unsigned revision = 0;
    std::string source;
};
std::map<Rml::ElementDocument *, DocumentState> documents;
Rml::SharedPtr<Rml::StyleSheetContainer> additions;
unsigned additionsRevision = 0;
int backgroundWidth = 0, backgroundHeight = 0;
Rml::ElementDocument *backgroundDocument = nullptr;
unsigned backgroundRevision = 0;
int lastBackgroundWidth = 0, lastBackgroundHeight = 0;

Rml::SharedPtr<Rml::StyleSheetContainer> parseSheet(const std::string &path)
{
    std::vector<unsigned char> bytes;
    if (!RmlInterfacePlatform::readFile(path, bytes)) return {};
    auto sheet = Rml::MakeShared<Rml::StyleSheetContainer>();
    Rml::StreamMemory stream(reinterpret_cast<const Rml::byte *>(bytes.data()), bytes.size());
    stream.SetSourceURL(path);
    if (!sheet->LoadStyleSheetContainer(&stream)) return {};
    return sheet;
}

}

const Values &current()
{
    if (!loaded && TheGlobalData)
    {
        UserPreferences pref;
        pref.load(preferenceFile);
        values.scalePercent = std::clamp(pref.getInt("Scale", 100), 50, 100);
        values.controlBarScalePercent = std::clamp(pref.getInt("ControlBarScale", 100), 50, 100);
        values.mobileMode = pref.getBool("MobileMode", FALSE);
        values.mobileUi = pref.getBool("MobileUI", FALSE);
        values.mobileMinimapPercent = std::clamp(pref.getInt("MobileMinimapScale", 100), 50, 150);
        values.darkTheme = pref.getBool("DarkTheme", FALSE);
        values.customTheme = pref.getAsciiString("CustomTheme", "").str();
        values.enableBackground = pref.getBool("EnableBackground", FALSE);
        values.backgroundImage = pref.getAsciiString("BackgroundImage", "").str();
        values.hideBorder = pref.getBool("HideBorder", FALSE);
        values.showEnergyNumbers = pref.getBool("ShowEnergyNumbers", FALSE);
        values.showUnitName = pref.getBool("ShowUnitName", FALSE);
        values.showGeneralRank = pref.getBool("ShowGeneralRank", FALSE);
        values.showPromotionPoints = pref.getBool("ShowPromotionPoints", FALSE);
        values.colorDisabledIcons = pref.getBool("ColorDisabledIcons", FALSE);
        values.showIconHotkeys = pref.getBool("ShowIconHotkeys", TRUE);
        values.productionProgressStyle = pref.getInt("ProductionProgressStyle", 0) == 1 ? 1 : 0;
        loaded = true;
    }
    return values;
}

int effectiveScalePercent() { return current().scalePercent + (current().mobileMode ? 30 : 0); }
int effectiveControlBarScalePercent() { return current().controlBarScalePercent + (current().mobileMode ? 30 : 0); }

std::string stylePath(const std::string &path)
{
    std::string normalized = path;
    for (char &c : normalized) if (c == '\\') c = '/';
    if (!normalized.empty() && normalized[0] == '/') normalized.erase(0, 1);
    if (normalized.size() > 3 && (normalized[0] == 'U' || normalized[0] == 'u') &&
        (normalized[1] == 'I' || normalized[1] == 'i') && normalized[2] == '/' &&
        normalized.find('/', 3) == std::string::npos && RmlInterfacePlatform::extension(normalized) == ".rcss")
        return std::string("UI/") + (current().darkTheme ? "DARK/" : "BLUE/") + normalized.substr(3);
    return path;
}

std::string scaleControlBarStyle(const std::string &source)
{
    const auto &settings = current();
    if (settings.controlBarScalePercent == settings.scalePercent) return source;
    const double scale = double(effectiveControlBarScalePercent()) / effectiveScalePercent();
    static const std::regex tokens(R"rcss(/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|(\bresolution\s*:\s*)([0-9]+(?:\.[0-9]+)?)(x\b)|(-?(?:[0-9]+(?:\.[0-9]+)?|\.[0-9]+))(dp\b))rcss");
    std::string result;
    size_t offset = 0;
    for (std::sregex_iterator it(source.begin(), source.end(), tokens), end; it != end; ++it)
    {
        const auto &match = *it;
        result.append(source, offset, size_t(match.position()) - offset);
        if (match[2].matched)
            result += match[1].str() + std::to_string(std::stod(match[2].str()) / scale) + match[3].str();
        else if (match[4].matched)
            result += std::to_string(std::stod(match[4].str()) * scale) + match[5].str();
        else result += match.str();
        offset = size_t(match.position() + match.length());
    }
    result.append(source, offset, std::string::npos);
    return result;
}

bool save(const Values &input, std::string &error)
{
    error.clear();
    Values next = input;
    next.scalePercent = std::clamp(next.scalePercent, 50, 100);
    next.controlBarScalePercent = std::clamp(next.controlBarScalePercent, 50, 100);
    next.mobileMinimapPercent = std::clamp(next.mobileMinimapPercent, 50, 150);
    next.productionProgressStyle = next.productionProgressStyle == 1 ? 1 : 0;
    if (!next.customTheme.empty() && (RmlInterfacePlatform::extension(next.customTheme) != ".rcss" || !parseSheet(next.customTheme)))
    {
        error = "Custom Theme could not be loaded. Choose a valid .rcss file.";
        return false;
    }
    if (next.enableBackground)
    {
        const std::string ext = RmlInterfacePlatform::extension(next.backgroundImage);
        int w = 0, h = 0;
        if ((ext != ".png" && ext != ".jpg" && ext != ".jpeg") || !RmlInterfacePlatform::imageSize(next.backgroundImage, w, h))
        {
            error = "Choose a readable PNG or JPG background image.";
            return false;
        }
    }
    UserPreferences pref;
    pref.load(preferenceFile);
    pref.setInt("Scale", next.scalePercent);
    pref.setInt("ControlBarScale", next.controlBarScalePercent);
    pref.setBool("MobileMode", next.mobileMode);
    pref.setBool("MobileUI", next.mobileUi);
    pref.setInt("MobileMinimapScale", next.mobileMinimapPercent);
    pref.setBool("DarkTheme", next.darkTheme);
    pref.setAsciiString("CustomTheme", next.customTheme.c_str());
    pref.setBool("EnableBackground", next.enableBackground);
    pref.setAsciiString("BackgroundImage", next.backgroundImage.c_str());
    pref.setBool("HideBorder", next.hideBorder);
    pref.setBool("ShowEnergyNumbers", next.showEnergyNumbers);
    pref.setBool("ShowUnitName", next.showUnitName);
    pref.setBool("ShowGeneralRank", next.showGeneralRank);
    pref.setBool("ShowPromotionPoints", next.showPromotionPoints);
    pref.setBool("ColorDisabledIcons", next.colorDisabledIcons);
    pref.setBool("ShowIconHotkeys", next.showIconHotkeys);
    pref.setInt("ProductionProgressStyle", next.productionProgressStyle);
    if (!pref.write())
    {
        error = "Could not save GeneralsOnlineInterface.ini in the game user-data folder.";
        return false;
    }
    values = next;
    loaded = true;
    ++revision;
    return true;
}

std::vector<std::string> themeFiles() { return RmlInterfacePlatform::catalog("Themes", false); }
std::vector<std::string> backgroundFiles() { return RmlInterfacePlatform::catalog("Backgrounds", true); }
std::string displayName(const std::string &path) { return RmlInterfacePlatform::displayName(path); }
bool readBackground(std::vector<unsigned char> &bytes) { return RmlInterfacePlatform::readFile(current().backgroundImage, bytes); }

std::string browseBackground() { return RmlInterfacePlatform::browseBackground(); }

void apply(Rml::Context *context, int width, int height)
{
    if (!context) return;
    const Values &settings = current();
    const float density = height / 1080.0f * effectiveScalePercent() / 100.0f;
    if (std::abs(context->GetDensityIndependentPixelRatio() - density) > 0.0001f)
        context->SetDensityIndependentPixelRatio(density);

    if (additionsRevision != revision)
    {
        Rml::Factory::ClearStyleSheetCache();
        additions.reset();
        if (!settings.customTheme.empty())
        {
            const auto custom = parseSheet(settings.customTheme);
            if (custom) additions = custom;
        }
        backgroundWidth = backgroundHeight = 0;
        if (settings.enableBackground)
        {
            RmlInterfacePlatform::imageSize(settings.backgroundImage, backgroundWidth, backgroundHeight);
        }
        additionsRevision = revision;
    }

    std::set<Rml::ElementDocument *> live;
    bool menuVisible = false;
    for (int i = 0; i < context->GetNumDocuments(); ++i)
    {
        auto *doc = context->GetDocument(i);
        if (doc && doc->IsVisible() && doc->IsClassSet("screen") && !doc->IsClassSet("ld"))
        {
            menuVisible = true;
            break;
        }
    }
    const bool showBackground = settings.enableBackground && menuVisible && backgroundWidth > 0 && backgroundHeight > 0;
    if (showBackground && !backgroundDocument)
    {


        backgroundDocument = context->CreateDocument();
        if (backgroundDocument)
        {
            backgroundDocument->SetProperty("width", "100%");
            backgroundDocument->SetProperty("height", "100%");
            backgroundDocument->SetProperty("pointer-events", "none");
            auto element = backgroundDocument->CreateElement("img");
            element->SetId("interface-background-image");
            element->SetProperty("position", "absolute");
            element->SetProperty("pointer-events", "none");
            backgroundDocument->AppendChild(std::move(element));
        }
    }
    if (backgroundDocument)
    {
        if (showBackground)
        {
            if (!backgroundDocument->IsVisible()) backgroundDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
            if (context->GetDocument(0) != backgroundDocument) backgroundDocument->PushToBack();
            if (backgroundRevision != revision || lastBackgroundWidth != width || lastBackgroundHeight != height)
            {
                auto *image = backgroundDocument->GetElementById("interface-background-image");
                image->SetAttribute("src", "/interface-background-" + std::to_string(revision) + ".png");
                const float cover = (std::max)(float(width) / backgroundWidth, float(height) / backgroundHeight);
                const float w = backgroundWidth * cover, h = backgroundHeight * cover;
                image->SetProperty("width", std::to_string(w) + "px");
                image->SetProperty("height", std::to_string(h) + "px");
                image->SetProperty("left", std::to_string((width - w) * 0.5f) + "px");
                image->SetProperty("top", std::to_string((height - h) * 0.5f) + "px");
                backgroundRevision = revision;
                lastBackgroundWidth = width;
                lastBackgroundHeight = height;
            }
        }
        else if (backgroundDocument->IsVisible()) backgroundDocument->Hide();
    }
    for (int i = 0; i < context->GetNumDocuments(); ++i)
    {
        auto *doc = context->GetDocument(i);
        if (!doc || doc == backgroundDocument) continue;
        live.insert(doc);
        auto &state = documents[doc];
        const bool appearanceChanged = state.revision != revision || state.source != doc->GetSourceURL();
        if (!state.base || appearanceChanged)
        {
            if (appearanceChanged && !doc->GetSourceURL().empty())
                RmlTextShaping::reloadStyles(doc);
            Rml::StyleSheetContainer empty;
            if (const auto *base = doc->GetStyleSheetContainer())
                state.base = base->CombineStyleSheetContainer(empty);
            state.source = doc->GetSourceURL();
            state.revision = 0;
        }
        if (state.base && appearanceChanged)
        {
            doc->SetStyleSheetContainer(additions ? state.base->CombineStyleSheetContainer(*additions) : state.base);
            state.revision = revision;
        }
        if (appearanceChanged)
        {
            Rml::ElementList rulers;
            doc->GetElementsByClassName(rulers, "screen-ruler");
            for (auto *ruler : rulers)
            {
                if (settings.hideBorder) ruler->SetProperty("display", "none");
                else ruler->RemoveProperty("display");
            }
        }
        if (doc->IsClassSet("cb"))
        {
            doc->SetClass("mobile-ui", settings.mobileUi);
            if (settings.mobileUi && appearanceChanged)
            {
                const float factor = float(effectiveControlBarScalePercent()) / effectiveScalePercent();
                const float radarSize = 240.0f * settings.mobileMinimapPercent / 100.0f;
                auto setDp = [factor](Rml::Element *element, const char *property, float value) {
                    if (element) element->SetProperty(property, std::to_string(value * factor) + "dp");
                };
                auto *radar = doc->GetElementById("mobile-radar");
                auto *map = doc->GetElementById("mobile-radar-map");
                setDp(radar, "width", radarSize);
                setDp(radar, "height", radarSize);
                setDp(map, "left", radarSize * 5 / 340);
                setDp(map, "top", radarSize * 13 / 340);
                setDp(map, "width", radarSize * 322 / 340);
                setDp(map, "height", radarSize * 322 / 340);
                setDp(doc->GetElementById("mobile-resources"), "left", radarSize);
            }
            doc->SetClass("show-energy-numbers", settings.showEnergyNumbers);
            doc->SetClass("show-unit-name", settings.showUnitName);
            doc->SetClass("show-general-rank", settings.showGeneralRank);
            doc->SetClass("show-promotion-points", settings.showPromotionPoints);
            doc->SetClass("hide-icon-hotkeys", !settings.showIconHotkeys);
            doc->SetClass("classic-radial-progress", settings.productionProgressStyle == 1);
            doc->RemoveProperty("transform");
            doc->RemoveProperty("transform-origin");
            doc->RemoveProperty("width");
            doc->RemoveProperty("height");
        }
        state.revision = revision;
    }
    for (auto it = documents.begin(); it != documents.end();)
        if (!live.count(it->first)) it = documents.erase(it); else ++it;
}

void reset()
{
    documents.clear();
    additions.reset();
    additionsRevision = 0;
    loaded = false;
    backgroundDocument = nullptr;
    backgroundRevision = 0;
}
}
