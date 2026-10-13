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

// FILE: ControlBarActions.cpp ////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/ControlBarActions.h"

#include "Common/AudioEventRTS.h"
#include "Common/GameAudio.h"
#include "Common/GameUtility.h"
#include "Common/GlobalData.h"
#include "Common/Radar.h"
#include "Common/MessageStream.h"
#include "GameLogic/GameLogic.h"
#include "GameClient/ControlBar.h"
#include "GameClient/ControlBarData.h"
#include "Common/NameKeyGenerator.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetTextEntry.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Mouse.h"
#include "GameClient/View.h"

namespace
{
	GameWindow *buttonWindow( const ControlBarButtonId &id )
	{
		return TheControlBar ? TheControlBar->getButtonWindow( id ) : nullptr;
	}

	// what the window manager sends along: the pointer's screen position
	WindowMsgData mousePosition()
	{
		const MouseIO *io = TheMouse ? TheMouse->getMouseStatus() : nullptr;
		return io ? SHORTTOLONG( io->pos.x, io->pos.y ) : 0;
	}

	// the window manager only gives input to a shown, enabled button (GameWindow::winPointInChild())
	Bool takesInput( GameWindow *win )
	{
		return !win->winIsHidden() && BitIsSet( win->winGetStatus(), WIN_STATUS_ENABLED );
	}
}

//-------------------------------------------------------------------------------------------------
void ControlBarActions::enter( const ControlBarButtonId &id )
{
	if( id.group == CBB_INFO )
		return;
	GameWindow *win = buttonWindow( id );
	if( win && takesInput( win ) )
		TheWindowManager->winSendInputMsg( win, GWM_MOUSE_ENTERING, mousePosition(), 0 );
}

void ControlBarActions::leave( const ControlBarButtonId &id )
{
	if( id.group == CBB_INFO )
		return;
	GameWindow *win = buttonWindow( id );
	if( win )
		TheWindowManager->winSendInputMsg( win, GWM_MOUSE_LEAVING, mousePosition(), 0 );
}

void ControlBarActions::press( const ControlBarButtonId &id, Bool right )
{
	if( id.group == CBB_INFO )
		return;
	GameWindow *win = buttonWindow( id );
	if( win && takesInput( win ) )
		TheWindowManager->winSendInputMsg( win, right ? GWM_RIGHT_DOWN : GWM_LEFT_DOWN, mousePosition(), 0 );
}

void ControlBarActions::release( const ControlBarButtonId &id, Bool right )
{
	if( id.group == CBB_INFO )
		return;
	GameWindow *win = buttonWindow( id );
	if( win == nullptr || win->winIsHidden() )
		return;

	if( !takesInput( win ) )
	{
		if( !right && TheAudio )
		{
			AudioEventRTS disabledClick( "GUIClickDisabled" );
			TheAudio->addAudioEvent( &disabledClick );
		}
		return;
	}

	TheWindowManager->winSendInputMsg( win, right ? GWM_RIGHT_UP : GWM_LEFT_UP, mousePosition(), 0 );
}

void ControlBarActions::hover( const ControlBarButtonId &id )
{
	GameWindow *win = buttonWindow( id );
	if( win && !win->winIsHidden() && win->winGetTooltipFunc() )
		win->winGetTooltipFunc()( win, win->winGetInstanceData(), mousePosition() );
}

void ControlBarActions::setBeaconText( const UnicodeString &text )
{
	static const NameKeyType textID = NAMEKEY( "ControlBar.wnd:EditBeaconText" );
	GameWindow *entry = TheWindowManager->winGetWindowFromId( nullptr, textID );
	if( entry == nullptr || entry->winIsHidden() )
		return;
	GadgetTextEntrySetText( entry, text );
	TheWindowManager->winSendSystemMsg( entry->winGetOwner(), GEM_EDIT_DONE, (WindowMsgData)entry, 0 );
}

void ControlBarActions::leaveBeacon()
{
	if( TheInGameUI )
		TheInGameUI->deselectAllDrawables(); // there is only the beacon selected
}

void ControlBarActions::enterSciencePanel()
{
	static const NameKeyType panelID = NAMEKEY( "GeneralsExpPoints.wnd:GenExpParent" );
	GameWindow *panel = TheWindowManager->winGetWindowFromId( nullptr, panelID );
	if( panel )
		TheWindowManager->winSendInputMsg( panel, GWM_MOUSE_ENTERING, mousePosition(), 0 );
}

void ControlBarActions::panelRelease()
{
	GameWinBlockInput( nullptr, GWM_LEFT_UP, mousePosition(), 0 );
}

//-------------------------------------------------------------------------------------------------
void ControlBarActions::placeRadar( Int x, Int y, Int width, Int height )
{
	GameWindow *win = TheControlBar ? TheControlBar->getRadarWindow() : nullptr;
	if( win == nullptr )
		return;

	ICoord2D pos, size;
	win->winGetScreenPosition( &pos.x, &pos.y );
	win->winGetSize( &size.x, &size.y );
	if( pos.x == x && pos.y == y && size.x == width && size.y == height )
		return;

	// winSetPosition() is relative to the parent
	ICoord2D local;
	win->winGetPosition( &local.x, &local.y );
	win->winSetPosition( local.x + x - pos.x, local.y + y - pos.y );
	win->winSetSize( width, height );
}

void ControlBarActions::drawRadar()
{
	GameWindow *win = TheControlBar ? TheControlBar->getRadarWindow() : nullptr;
	if( win && win->winGetDrawFunc() )
		win->winGetDrawFunc()( win, win->winGetInstanceData() );
}

void ControlBarActions::radarInput( UnsignedInt message )
{
	GameWindow *win = TheControlBar ? TheControlBar->getRadarWindow() : nullptr;
	if( win )
		TheWindowManager->winSendInputMsg( win, message, mousePosition(), 0 );
}

void ControlBarActions::radarDrag( Bool right )
{
	GameWindow *win = TheControlBar ? TheControlBar->getRadarWindow() : nullptr;
	const MouseIO *io = TheMouse ? TheMouse->getMouseStatus() : nullptr;
	if( win == nullptr || io == nullptr || !rts::localPlayerHasRadar() )
		return;

	const DrawableList *drawables = TheInGameUI->getAllSelectedLocalDrawables();
	const Bool looks = drawables->empty() || ( TheGlobalData->m_useAlternateMouse ? !right : right );
	if( !looks )
		return;

	// LeftHUDInput's look at, at the pointer
	ICoord2D screenPos, mouse, radar;
	win->winGetScreenPosition( &screenPos.x, &screenPos.y );
	mouse.x = io->pos.x - screenPos.x;
	mouse.y = io->pos.y - screenPos.y;
	Coord3D world;
	if( TheRadar->localPixelToRadar( &mouse, &radar ) && TheRadar->radarToWorld( &radar, &world ) )
		TheTacticalView->userLookAt( &world );
}

void ControlBarActions::mobileAction(MobileAction action)
{
    if (!TheInGameUI || !TheMessageStream || !TheGameLogic || !TheGameLogic->isInGame() ||
        TheGameLogic->isInShellGame() || TheGameLogic->isInReplayGame()) return;
    if (action == MobileAction::Cancel)
    {
        const Bool pending = TheInGameUI->getGUICommand() || TheInGameUI->getPendingPlaceType() || TheInGameUI->isInAttackMoveToMode() || TheInGameUI->isInWaypointMode();
        TheInGameUI->setGUICommand(nullptr);
        TheInGameUI->placeBuildAvailable(nullptr, nullptr);
        TheInGameUI->clearAttackMoveToMode();
        if (TheInGameUI->isInWaypointMode()) TheMessageStream->appendMessage(GameMessage::MSG_META_END_WAYPOINTS);
        TheInGameUI->setScrolling(FALSE);
        if (!pending) TheInGameUI->deselectAllDrawables();
        return;
    }
    GameMessage::Type message;
    switch (action)
    {
        case MobileAction::SelectAll: message = GameMessage::MSG_META_SELECT_ALL; break;
        case MobileAction::SelectMatching: message = GameMessage::MSG_META_SELECT_MATCHING_UNITS; break;
        case MobileAction::Formation: message = GameMessage::MSG_META_CREATE_FORMATION; break;
        case MobileAction::Scatter: message = GameMessage::MSG_META_SCATTER; break;
        case MobileAction::Stop: message = GameMessage::MSG_META_STOP; break;
        case MobileAction::AttackMove: message = GameMessage::MSG_META_TOGGLE_ATTACKMOVE; break;
        case MobileAction::Waypoints: message = TheInGameUI->isInWaypointMode() ? GameMessage::MSG_META_END_WAYPOINTS : GameMessage::MSG_META_BEGIN_WAYPOINTS; break;
        case MobileAction::Chat: message = GameMessage::MSG_META_CHAT_EVERYONE; break;
        default: return;
    }
    TheMessageStream->appendMessage(message);
}

void ControlBarActions::selectMobileGroup(Int group)
{
    if (group < 0 || group > 9 || !TheMessageStream || !TheGameLogic ||
        !TheGameLogic->isInGame() || TheGameLogic->isInShellGame() || TheGameLogic->isInReplayGame()) return;
    TheMessageStream->appendMessage(static_cast<GameMessage::Type>(GameMessage::MSG_META_SELECT_TEAM0 + group));
}
