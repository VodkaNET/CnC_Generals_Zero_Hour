/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "W3DDevice/GameClient/RmlUi/RmlOptionsScreen.h"

#include "Common/OptionPreferences.h"
#include "Common/UnicodeUtf8.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/ShellHooks.h"
#include "GameClient/Shell.h"
#include "GameClient/Display.h"
#include "W3DDevice/GameClient/RmlUi/RmlUiManager.h"
#include "W3DDevice/GameClient/RmlUi/RmlInterfaceSettings.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>

#include <functional>
#include <algorithm>

namespace
{
int selectInterfacePath(std::vector<std::string> &paths, const std::string &path)
{
    if (path.empty()) return 0;
    auto it = std::find(paths.begin(), paths.end(), path);
    if (it == paths.end()) { paths.push_back(path); return int(paths.size()); }
    return int(it - paths.begin()) + 1;
}
Rml::String interfacePathLabel(const std::string &path)
{
    Rml::String result;
    for (char c : RmlInterfaceSettings::displayName(path))
        switch (c) { case '&': result += "&amp;"; break; case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break; case '"': result += "&quot;"; break; default: result += c; }
    return result;
}
}

extern void DoResolutionDialog(); // MainMenu.cpp; same resolution-change confirm dialog as the .wnd Accept handler

//-------------------------------------------------------------------------------------------------
RmlOptionsScreen::RmlOptionsScreen()
{
}

RmlOptionsScreen::~RmlOptionsScreen()
{
	if (m_context && m_document)
	{
		m_context->UnloadDocument(m_document);
		m_context->RemoveDataModel("options");
	}
	delete m_pref;
}

void RmlOptionsScreen::load(Rml::Context *context)
{
	if (m_document)
		return; // already loaded

	m_context = context;
	m_pref = NEW OptionPreferences;

	Rml::DataModelConstructor constructor = context->CreateDataModel("options");
	if (constructor)
	{
		m_hq.bind(constructor);
		constructor.Bind("tab", &m_model.tab);
		constructor.Bind("ui_scale", &m_model.uiScale);
		constructor.Bind("mobile_mode", &m_model.mobileMode);
		constructor.Bind("mobile_ui", &m_model.mobileUi);
		constructor.Bind("mobile_minimap_scale", &m_model.mobileMinimapScale);
		constructor.Bind("controlbar_scale", &m_model.controlBarScale);
		constructor.Bind("ui_theme", &m_model.uiTheme);
		constructor.Bind("custom_theme_index", &m_model.customThemeIndex);
		constructor.Bind("interface_background", &m_model.enableBackground);
		constructor.Bind("background_index", &m_model.backgroundIndex);
		constructor.Bind("hide_ui_border", &m_model.hideBorder);
		constructor.Bind("show_energy_numbers", &m_model.showEnergyNumbers);
		constructor.Bind("show_unit_name", &m_model.showUnitName);
		constructor.Bind("show_general_rank", &m_model.showGeneralRank);
		constructor.Bind("show_promotion_points", &m_model.showPromotionPoints);
		constructor.Bind("show_icon_hotkeys", &m_model.showIconHotkeys);
		constructor.Bind("production_progress_style", &m_model.productionProgressStyle);
		constructor.Bind("interface_error", &m_model.interfaceError);
		constructor.Bind("background_path", &m_model.backgroundPath);
		constructor.Bind("language_filter", &m_model.languageFilter);
		constructor.Bind("send_delay", &m_model.sendDelay);
		constructor.Bind("scroll_speed", &m_model.scrollSpeed);
		constructor.Bind("music_volume", &m_model.musicVolume);
		constructor.Bind("sfx_volume", &m_model.sfxVolume);
		constructor.Bind("voice_volume", &m_model.voiceVolume);
		constructor.Bind("alternate_mouse", &m_model.alternateMouse);
		constructor.Bind("retaliation", &m_model.retaliation);
		constructor.Bind("double_click_attack_move", &m_model.doubleClickAttackMove);
		constructor.Bind("text_language", &m_model.textLanguage);
		constructor.Bind("interface_width", &m_model.interfaceWidth);
		constructor.Bind("anti_aliasing", &m_model.antiAliasing);
		constructor.Bind("texture_filter", &m_model.textureFilter);
		constructor.Bind("anisotropy", &m_model.anisotropy);
		constructor.Bind("brightness", &m_model.brightness);

		constructor.Bind("resolution_index", &m_model.resolutionIndex);
		constructor.Bind("detail_level", &m_model.detailLevel);
		constructor.Bind("texture_resolution", &m_model.textureResolutionSliderPos);
		constructor.Bind("particle_cap", &m_model.particleCap);
		constructor.Bind("shadow_3d", &m_model.shadow3D);
		constructor.Bind("shadow_2d", &m_model.shadow2D);
		constructor.Bind("cloud_shadows", &m_model.cloudShadows);
		constructor.Bind("ground_lighting", &m_model.groundLighting);
		constructor.Bind("smooth_water", &m_model.smoothWater);
		constructor.Bind("extra_animations", &m_model.extraAnimations);
		constructor.Bind("no_dynamic_lod", &m_model.noDynamicLod);
		constructor.Bind("heat_effects", &m_model.heatEffects);
		constructor.Bind("building_occlusion", &m_model.buildingOcclusion);
		constructor.Bind("props", &m_model.props);

		constructor.Bind("lan_ip_index", &m_model.lanIPIndex);
		constructor.Bind("online_ip_index", &m_model.onlineIPIndex);
		constructor.Bind("http_proxy", &m_model.httpProxy);
		constructor.Bind("firewall_port_override", &m_model.firewallPortOverride);

		constructor.Bind("restricted", &m_model.restricted);
		constructor.Bind("detail_controls_disabled", &m_model.detailControlsDisabled);

		constructor.BindEventCallback("select_video", &RmlOptionsScreen::onSelectVideo, this);
		constructor.BindEventCallback("select_graphics", &RmlOptionsScreen::onSelectGraphics, this);
		constructor.BindEventCallback("select_audio", &RmlOptionsScreen::onSelectAudio, this);
		constructor.BindEventCallback("select_controls", &RmlOptionsScreen::onSelectControls, this);
		constructor.BindEventCallback("select_network", &RmlOptionsScreen::onSelectNetwork, this);
		constructor.BindEventCallback("select_interface", &RmlOptionsScreen::onSelectInterface, this);
		constructor.BindEventCallback("browse_background", &RmlOptionsScreen::onBrowseBackground, this);
		constructor.BindEventCallback("background_changed", &RmlOptionsScreen::onBackgroundChanged, this);
		constructor.BindEventCallback("accept", &RmlOptionsScreen::onAccept, this);
		constructor.BindEventCallback("cancel", &RmlOptionsScreen::onCancel, this);
		constructor.BindEventCallback("defaults", &RmlOptionsScreen::onDefaults, this);
		constructor.BindEventCallback("detail_level_changed", &RmlOptionsScreen::onDetailLevelChanged, this);
		constructor.BindEventCallback("detail_control_changed", &RmlOptionsScreen::onDetailControlChanged, this);
		constructor.BindEventCallback("firewall_refresh", &RmlOptionsScreen::onFirewallRefresh, this);

		m_modelHandle = constructor.GetModelHandle();
	}

	m_document = context->LoadDocument("UI/Options.rml");
}

void RmlOptionsScreen::show()
{
	if (!m_document || isVisible())
		return;

	loadCurrentValues();
	populateSelectOptions();

	// An overlay like the .wnd (runInit/hide(FALSE)/bringForward): whatever opened it stays visible
	// behind. Modal keeps it from taking hover/focus.
	m_hq.refresh(m_modelHandle, true);
	m_document->Show(Rml::ModalFlag::Modal);
	SignalUIInteraction(SHELL_SCRIPT_HOOK_OPTIONS_OPENED);
}

void RmlOptionsScreen::hide()
{
	if (!isVisible())
		return;

	m_document->Hide();
	SignalUIInteraction(SHELL_SCRIPT_HOOK_OPTIONS_CLOSED);
}

bool RmlOptionsScreen::isVisible() const
{
	return m_document && m_document->IsVisible();
}

void RmlOptionsScreen::onBack()
{
	// Escape behaves like Cancel: OptionsMenu.cpp's buttonBack handler (GBM_SELECTED) never
	// calls saveOptions() -- it only deletes its OptionPreferences and destroys the layout.
	// Nothing in this menu applies live before Accept either (GSM_SLIDER_TRACK/GCM_SELECTED
	// only flip the detail combo to Custom, a UI-only state), so discarding here is exact,
	// not an approximation.
	hide();
	if (TheShell)
		TheShell->destroyOptionsLayout();
}

//-------------------------------------------------------------------------------------------------
void RmlOptionsScreen::loadDetailPresetValues(const OptionsValues::DetailPresetValues &values)
{
	m_model.textureResolutionSliderPos = values.textureResolutionSliderPos;
	m_model.particleCap = values.particleCap;
	m_model.shadow3D = values.shadow3D;
	m_model.shadow2D = values.shadow2D;
	m_model.cloudShadows = values.cloudShadows;
	m_model.groundLighting = values.groundLighting;
	m_model.smoothWater = values.smoothWater;
	m_model.extraAnimations = values.extraAnimations;
	m_model.noDynamicLod = values.noDynamicLod;
	m_model.heatEffects = values.heatEffects;
	m_model.buildingOcclusion = values.buildingOcclusion;
	m_model.props = values.props;
}

void RmlOptionsScreen::updateDetailControlsDisabled()
{
	// "Without a changeable preset, only an existing Custom detail can be edited" (OptionsMenuInit).
	m_model.detailControlsDisabled = m_model.restricted && m_model.detailLevel != OptionsValues::GetDetailCustomLevel();
}

void RmlOptionsScreen::loadCurrentValues()
{
	const auto &interfaceValues = RmlInterfaceSettings::current();
	m_model.mobileMode = interfaceValues.mobileMode;
	m_model.mobileUi = interfaceValues.mobileUi;
	m_model.mobileMinimapScale = interfaceValues.mobileMinimapPercent;
	m_model.uiScale = interfaceValues.scalePercent;
	m_model.controlBarScale = interfaceValues.controlBarScalePercent;
	m_model.uiTheme = interfaceValues.darkTheme ? 1 : 0;
	m_customThemes = RmlInterfaceSettings::themeFiles();
	m_backgroundImages = RmlInterfaceSettings::backgroundFiles();
	m_model.customThemeIndex = selectInterfacePath(m_customThemes, interfaceValues.customTheme);
	m_model.enableBackground = interfaceValues.enableBackground;
	m_model.backgroundIndex = selectInterfacePath(m_backgroundImages, interfaceValues.backgroundImage);
	m_model.backgroundPath = interfaceValues.backgroundImage;
	m_model.hideBorder = interfaceValues.hideBorder;
	m_model.showEnergyNumbers = interfaceValues.showEnergyNumbers;
	m_model.showUnitName = interfaceValues.showUnitName;
	m_model.showGeneralRank = interfaceValues.showGeneralRank;
	m_model.showPromotionPoints = interfaceValues.showPromotionPoints;
	m_model.showIconHotkeys = interfaceValues.showIconHotkeys;
	m_model.productionProgressStyle = interfaceValues.productionProgressStyle;
	m_model.interfaceError.clear();
	m_model.languageFilter = OptionsValues::GetCurrentLanguageFilter();
	m_model.sendDelay = OptionsValues::GetCurrentSendDelay();
	m_model.scrollSpeed = OptionsValues::GetCurrentScrollSpeedPercent();
	m_model.musicVolume = OptionsValues::GetCurrentMusicVolumePercent();
	m_model.sfxVolume = OptionsValues::GetCurrentSFXVolumePercent();
	m_model.voiceVolume = OptionsValues::GetCurrentVoiceVolumePercent();
	m_initialMusicVolume = m_model.musicVolume;
	m_initialSfxVolume = m_model.sfxVolume;
	m_initialVoiceVolume = m_model.voiceVolume;
	m_model.alternateMouse = OptionsValues::GetCurrentAlternateMouse();
	m_model.retaliation = OptionsValues::GetCurrentRetaliation();
	m_model.doubleClickAttackMove = OptionsValues::GetCurrentDoubleClickAttackMove();
	m_model.textLanguage = OptionsValues::GetCurrentTextLanguageChoice();
	m_model.interfaceWidth = OptionsValues::GetCurrentInterfaceWidth();
	m_model.antiAliasing = OptionsValues::GetCurrentAntiAliasingIndex();
	m_model.textureFilter = OptionsValues::GetCurrentTextureFilterIndex();
	m_model.anisotropy = OptionsValues::GetCurrentAnisotropyIndex();
	m_model.brightness = OptionsValues::GetCurrentBrightnessPercent(*m_pref);

	m_model.resolutionIndex = OptionsValues::GetCurrentDisplayModeIndex();
	m_model.detailLevel = OptionsValues::GetCurrentDetailLevel();
	m_applyingDetailPreset = true; // loading straight from TheGlobalData is not "editing"
	loadDetailPresetValues(OptionsValues::GetCurrentGraphicsValues());
	m_applyingDetailPreset = false;

	m_model.lanIPIndex = OptionsValues::GetCurrentLANIPIndex(*m_pref);
	m_model.onlineIPIndex = OptionsValues::GetCurrentOnlineIPIndex(*m_pref);
	m_model.httpProxy = OptionsValues::GetCurrentHTTPProxy().str();
	Int port = OptionsValues::GetCurrentFirewallPortOverride();
	AsciiString portStr;
	if (port != 0)
		portStr.format("%d", port);
	m_model.firewallPortOverride = portStr.str();

	m_model.restricted = OptionsValues::IsOptionsRestrictedContext();
	updateDetailControlsDisabled();

	if (m_modelHandle)
		m_modelHandle.DirtyAllVariables();
}

void RmlOptionsScreen::loadDefaultValues()
{
	m_model.uiScale = 100;
	m_model.mobileMode = false;
	m_model.mobileUi = false;
	m_model.mobileMinimapScale = 100;
	m_model.controlBarScale = 100;
	m_model.uiTheme = 0;
	m_model.customThemeIndex = 0;
	m_model.enableBackground = false;
	m_model.backgroundIndex = 0;
	m_model.backgroundPath.clear();
	m_model.hideBorder = false;
	m_model.showEnergyNumbers = false;
	m_model.showUnitName = false;
	m_model.showGeneralRank = false;
	m_model.showPromotionPoints = false;
	m_model.showIconHotkeys = true;
	m_model.productionProgressStyle = 0;
	m_model.interfaceError.clear();
	m_model.languageFilter = OptionsValues::GetDefaultLanguageFilter();
	m_model.sendDelay = OptionsValues::GetDefaultSendDelay();
	m_model.scrollSpeed = OptionsValues::GetDefaultScrollSpeedPercent();
	m_model.musicVolume = OptionsValues::GetDefaultMusicVolumePercent();
	m_model.sfxVolume = OptionsValues::GetDefaultSFXVolumePercent();
	m_model.voiceVolume = OptionsValues::GetDefaultVoiceVolumePercent();
	m_model.alternateMouse = OptionsValues::GetDefaultAlternateMouse();
	m_model.retaliation = OptionsValues::GetDefaultRetaliation();
	m_model.doubleClickAttackMove = OptionsValues::GetDefaultDoubleClickAttackMove();
	m_model.brightness = OptionsValues::GetDefaultBrightnessPercent();
	// Anti-aliasing/texture filter/anisotropy/resolution/detail have no shared "default" (the
	// .wnd version's setDefaults() never resets these either -- see its ModifyDisplaySettings
	// compile-time gate), so they are left exactly as currently applied.

	if (m_modelHandle)
		m_modelHandle.DirtyAllVariables();
}

bool RmlOptionsScreen::applyAndSave()
{
	OptionsValues::ApplyLanguageFilter(*m_pref, m_model.languageFilter);

	// Every gate below mirrors a winGetEnabled() check the .wnd Accept handler makes; restricted
	// (in-game/online) disables the same set of controls there (see OptionsMenuInit).
	if (!m_model.restricted)
		OptionsValues::ApplySendDelay(*m_pref, m_model.sendDelay);

	OptionsValues::ApplyScrollSpeedPercent(*m_pref, m_model.scrollSpeed);
	if (m_model.musicVolume != m_initialMusicVolume)
		OptionsValues::ApplyMusicVolumePercent(*m_pref, m_model.musicVolume);
	if (m_model.sfxVolume != m_initialSfxVolume)
		OptionsValues::ApplySFXVolumePercent(*m_pref, m_model.sfxVolume);
	if (m_model.voiceVolume != m_initialVoiceVolume)
		OptionsValues::ApplyVoiceVolumePercent(*m_pref, m_model.voiceVolume);
	OptionsValues::ApplyMouseOptions(*m_pref, m_model.alternateMouse, m_model.retaliation, m_model.doubleClickAttackMove);
	OptionsValues::ApplyTextLanguageChoice(m_model.textLanguage);
	OptionsValues::ApplyInterfaceWidth(m_model.interfaceWidth); // RmlUiManager::update() applies it to every open screen
	OptionsValues::ApplyAntiAliasing(*m_pref, m_model.antiAliasing);
	OptionsValues::ApplyTextureFilter(*m_pref, m_model.textureFilter);
	OptionsValues::ApplyAnisotropy(*m_pref, m_model.anisotropy);
	OptionsValues::ApplyBrightnessPercent(*m_pref, m_model.brightness);

	// Custom detail values apply whenever the level is Custom, regardless of restriction --
	// matches saveOptions()'s "if (index == STATIC_GAME_LOD_CUSTOM)" block, which is not gated
	// on comboBoxDetail's enabled state.
	if (m_model.detailLevel == OptionsValues::GetDetailCustomLevel())
	{
		OptionsValues::DetailPresetValues values;
		values.textureResolutionSliderPos = m_model.textureResolutionSliderPos;
		values.particleCap = m_model.particleCap;
		values.shadow3D = m_model.shadow3D;
		values.shadow2D = m_model.shadow2D;
		values.cloudShadows = m_model.cloudShadows;
		values.groundLighting = m_model.groundLighting;
		values.smoothWater = m_model.smoothWater;
		values.extraAnimations = m_model.extraAnimations;
		values.noDynamicLod = m_model.noDynamicLod;
		values.heatEffects = m_model.heatEffects;
		values.buildingOcclusion = m_model.buildingOcclusion;
		values.props = m_model.props;
		OptionsValues::ApplyCustomGraphicsSettings(*m_pref, values);
	}

	if (!m_model.restricted)
		OptionsValues::ApplyDetailLevel(*m_pref, m_model.detailLevel);

	if (!m_model.restricted)
	{
		OptionsValues::ApplyLANIPChoice(*m_pref, OptionsValues::GetIPChoiceIP(m_model.lanIPIndex));
		OptionsValues::ApplyOnlineIPChoice(*m_pref, OptionsValues::GetIPChoiceIP(m_model.onlineIPIndex));
		OptionsValues::ApplyHTTPProxy(AsciiString(m_model.httpProxy.c_str()));
		OptionsValues::ApplyFirewallPortOverride(*m_pref, atoi(m_model.firewallPortOverride.c_str()));
	}

	// Resolution must be applied dead last, before the write: it can recreate the shell (see
	// ApplyDisplayMode()'s comment), and its pref entry still needs to make it into this write().
	// The caller (onAccept) shows the resolution confirm dialog only after hiding this screen and
	// destroying the options layout, exactly matching the .wnd Accept handler's own order.
	bool resolutionChanged = false;
	if (!m_model.restricted)
		resolutionChanged = OptionsValues::ApplyDisplayMode(*m_pref, m_model.resolutionIndex);

	m_pref->write();

	return resolutionChanged;
}

//-------------------------------------------------------------------------------------------------
void RmlOptionsScreen::populateSelectOptions()
{
	const auto values = m_model;
    auto fillSelect = [this](const char *id, int count, int selected, const std::function<Rml::String(int)> &label)
    {
        auto *element = m_document->GetElementById(id);
        if (!element || element->GetTagName() != "select") return;
        auto *select = static_cast<Rml::ElementFormControlSelect *>(element);
        select->RemoveAll();
        if (std::string(id) == "select_resolution" && selected < 0)
            select->Add(std::to_string(TheDisplay->getWidth()) + " x " + std::to_string(TheDisplay->getHeight()) + " (Current)", "-1");
        for (int i = 0; i < count; ++i)
        {
            const auto text = label(i);
            if (!text.empty()) select->Add(text, std::to_string(i));
        }
        select->SetValue(std::to_string(selected));
    };

    fillSelect("select_resolution", OptionsValues::GetDisplayModeCount(), values.resolutionIndex,
		[](int i)
		{
			OptionsValues::DisplayModeValues mode = OptionsValues::GetDisplayMode(i);
			return std::to_string(mode.width) + " x " + std::to_string(mode.height);
		});

	const int customLevel = OptionsValues::GetDetailCustomLevel();
	fillSelect("select_detail", customLevel + 1, values.detailLevel,
		[customLevel](int i)
		{
			// The .wnd's comboBoxDetail labels (OptionsMenu.cpp), which the LOD names only stand in for.
			static const char *const levelKeys[] = { "GUI:Low", "GUI:Medium", "GUI:High", "GO:GUI:VeryHigh" };
			if (i == customLevel)
				return Rml::String("<gametext key=\"GUI:Custom\"/>");
			if (i >= 0 && i < (int)(sizeof(levelKeys) / sizeof(levelKeys[0])))
				return Rml::String("<gametext key=\"") + levelKeys[i] + "\"/>";
			return Rml::String(OptionsValues::GetDetailLevelName(i).str());
		});

	// Each language by its own name (Arabic in its own direction), the installed one marked; choice 0, for an
	// installed language without an entry, is only listed then.
	fillSelect("select_language", OptionsValues::GetTextLanguageChoiceCount(), values.textLanguage,
		[](int i)
		{
			if (i == 0)
				return OptionsValues::GetInstalledTextLanguageChoice() ? Rml::String() : Rml::String("<gametext key=\"GO:GUI:LanguageInstalled\"/>");
			return unicodeToUtf8(OptionsValues::GetTextLanguageChoiceName(i));
		});

	fillSelect("select_lan_ip", OptionsValues::GetIPChoiceCount(), values.lanIPIndex,
		[](int i) { return Rml::String(OptionsValues::GetIPChoiceLabel(i).str()); });

	fillSelect("select_online_ip", OptionsValues::GetIPChoiceCount(), values.onlineIPIndex,
		[](int i) { return Rml::String(OptionsValues::GetIPChoiceLabel(i).str()); });
	fillSelect("select_custom_theme", int(m_customThemes.size()) + 1, values.customThemeIndex,
		[this](int i) { return i == 0 ? Rml::String("None") : interfacePathLabel(m_customThemes[i - 1]); });
	fillSelect("select_background", int(m_backgroundImages.size()) + 1, values.backgroundIndex,
		[this](int i) { return i == 0 ? Rml::String("Select an image...") : interfacePathLabel(m_backgroundImages[i - 1]); });
    m_model = values;
    if (m_modelHandle) m_modelHandle.DirtyAllVariables();
}

//-------------------------------------------------------------------------------------------------
void RmlOptionsScreen::onSelectTab(const Rml::String &tab)
{
	m_model.tab = tab;
	if (m_modelHandle)
		m_modelHandle.DirtyVariable("tab");
}

bool RmlOptionsScreen::saveInterfaceValues()
{
    RmlInterfaceSettings::Values next;
    next.mobileMode = m_model.mobileMode;
    next.mobileUi = m_model.mobileUi;
    next.mobileMinimapPercent = m_model.mobileMinimapScale;
    next.scalePercent = m_model.uiScale;
    next.controlBarScalePercent = m_model.controlBarScale;
    next.darkTheme = m_model.uiTheme == 1;
    next.enableBackground = m_model.enableBackground;
    next.hideBorder = m_model.hideBorder;
    next.showEnergyNumbers = m_model.showEnergyNumbers;
    next.showUnitName = m_model.showUnitName;
    next.showGeneralRank = m_model.showGeneralRank;
    next.showPromotionPoints = m_model.showPromotionPoints;
    next.showIconHotkeys = m_model.showIconHotkeys;
    next.productionProgressStyle = m_model.productionProgressStyle;
    if (m_model.customThemeIndex > 0 && m_model.customThemeIndex <= int(m_customThemes.size()))
        next.customTheme = m_customThemes[m_model.customThemeIndex - 1];
    if (m_model.backgroundIndex > 0 && m_model.backgroundIndex <= int(m_backgroundImages.size()))
        next.backgroundImage = m_backgroundImages[m_model.backgroundIndex - 1];
    std::string error;
    if (RmlInterfaceSettings::save(next, error)) return true;
    m_model.interfaceError = error;
    m_model.tab = "interface";
    if (m_modelHandle) m_modelHandle.DirtyAllVariables();
    return false;
}

void RmlOptionsScreen::onBrowseBackground(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
    const std::string path = RmlInterfaceSettings::browseBackground();
    if (path.empty()) return;
    m_model.backgroundIndex = selectInterfacePath(m_backgroundImages, path);
    m_model.backgroundPath = path;
    m_model.interfaceError.clear();
    populateSelectOptions();
    if (m_modelHandle) m_modelHandle.DirtyAllVariables();
}

void RmlOptionsScreen::onBackgroundChanged(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
    m_model.backgroundPath = m_model.backgroundIndex > 0 && m_model.backgroundIndex <= int(m_backgroundImages.size())
        ? m_backgroundImages[m_model.backgroundIndex - 1] : std::string();
    m_model.interfaceError.clear();
    if (m_modelHandle) m_modelHandle.DirtyAllVariables();
}

void RmlOptionsScreen::onAccept(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	if (!saveInterfaceValues()) return;
	const bool resolutionChanged = applyAndSave();
	hide();
	if (TheShell)
		TheShell->destroyOptionsLayout(); // symmetric with the .wnd Accept button (see MainMenu.cpp)

	// Same order as the .wnd Accept handler: destroy the layout, then show the confirm dialog.
	if (resolutionChanged)
		DoResolutionDialog();
}

void RmlOptionsScreen::onCancel(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	hide();
	if (TheShell)
		TheShell->destroyOptionsLayout();
}

void RmlOptionsScreen::onDefaults(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	loadDefaultValues();
}

void RmlOptionsScreen::onDetailLevelChanged(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	// Mirrors GCM_SELECTED's showDetailPreset(): choosing a named preset previews its values
	// without itself counting as an edit; choosing Custom directly leaves values untouched.
	const int customLevel = OptionsValues::GetDetailCustomLevel();
	if (m_model.detailLevel >= 0 && m_model.detailLevel < customLevel)
	{
		m_applyingDetailPreset = true;
		loadDetailPresetValues(OptionsValues::GetDetailPresetPreview(m_model.detailLevel));
		m_applyingDetailPreset = false;
	}

	updateDetailControlsDisabled();
	if (m_modelHandle)
		m_modelHandle.DirtyAllVariables();
}

void RmlOptionsScreen::onDetailControlChanged(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	// Mirrors switchDetailToCustom(): editing any detail-preset control switches the combo to
	// Custom, unless this edit is itself the result of applying a preset preview above, or the
	// controls are disabled (restricted, non-Custom -- can't be edited at all in that state).
	if (m_applyingDetailPreset || m_model.detailControlsDisabled)
		return;
	if (m_model.detailLevel == OptionsValues::GetDetailCustomLevel())
		return;

	m_model.detailLevel = OptionsValues::GetDetailCustomLevel();
	if (m_modelHandle)
		m_modelHandle.DirtyVariable("detail_level");
}

void RmlOptionsScreen::onFirewallRefresh(Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &)
{
	OptionsValues::RefreshFirewallBehavior(*m_pref);
}

//-------------------------------------------------------------------------------------------------
RmlOptionsScreen &RmlOptionsScreen::instance()
{
	static RmlOptionsScreen s_screen;
	return s_screen;
}

void OpenRmlOptionsScreen()
{
	if (!TheRmlUiManager)
		return;

	RmlOptionsScreen &screen = RmlOptionsScreen::instance();
	screen.load(TheRmlUiManager->getContext());
	screen.show();
}

void CloseRmlOptionsScreen()
{
	RmlOptionsScreen::instance().hide();
}

//-------------------------------------------------------------------------------------------------
void RmlOptionsScreen::update()
{
	m_hq.refresh(m_modelHandle);
}
