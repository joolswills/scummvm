/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"
#include "mads/game.h"
#include "mads/mads.h"
#include "mads/resources.h"
#include "mads/screen.h"
#include "mads/nebular/menu_nebular.h"

namespace MADS {

namespace Nebular {

#define NEBULAR_MENUSCREEN 990
#define MADS_MENU_Y ((MADS_SCREEN_HEIGHT - MADS_SCENE_HEIGHT) / 2)
#define MADS_MENU_ANIM_DELAY 70

MenuView::MenuView(MADSEngine *vm) : FullScreenDialog(vm) {
	_breakFlag = false;
	_redrawFlag = true;
	_palFlag = false;
}

void MenuView::show() {
	Scene &scene = _vm->_game->_scene;	
	EventsManager &events = *_vm->_events;
	display();

	events.setEventTarget(this);
	events.hideCursor();

	while (!_breakFlag && !_vm->shouldQuit()) {
		if (_redrawFlag) {
			scene.drawElements(_vm->_game->_fx, _vm->_game->_fx);

			_vm->_screen.copyRectToScreen(Common::Rect(0, 0, 320, 200));
			_redrawFlag = false;
		}

		_vm->_events->waitForNextFrame();
		_vm->_game->_fx = kTransitionNone;
		doFrame();
	}

	events.setEventTarget(nullptr);
}

void MenuView::display() {
	_vm->_palette->resetGamePalette(4, 8);

	FullScreenDialog::display();
}

/*------------------------------------------------------------------------*/

MainMenu::MainMenu(MADSEngine *vm): MenuView(vm) {
	Common::fill(&_menuItems[0], &_menuItems[7], (SpriteAsset *)nullptr);
	Common::fill(&_menuItemIndexes[0], &_menuItemIndexes[7], -1);
	_delayTimeout = 0;
	_menuItemIndex = -1;
	_frameIndex = 0;
	_skipFlag = false;
	_highlightedIndex = -1;
	_selectedIndex = -1;
	_buttonDown = false;
}

MainMenu::~MainMenu() {
}

void MainMenu::display() {
	MenuView::display();
	Scene &scene = _vm->_game->_scene;
	ScreenObjects &screenObjects = _vm->_game->_screenObjects;
	screenObjects.clear();

	// Load each of the menu item assets and add to the scene sprites list
	for (int i = 0; i < 7; ++i) {
		Common::String spritesName = Resources::formatName(NEBULAR_MENUSCREEN,
			'A', i + 1, EXT_SS, "");
		_menuItems[i] = new SpriteAsset(_vm, spritesName, 0);
		_menuItemIndexes[i] = scene._sprites.add(_menuItems[i]);

		// Register the menu item area in the screen objects
		MSprite *frame0 = _menuItems[i]->getFrame(0);
		Common::Point pt(frame0->_offset.x - (frame0->w / 2),
			frame0->_offset.y - frame0->h + _vm->_screen._offset.y);
		screenObjects.add(
			Common::Rect(pt.x, pt.y, pt.x + frame0->w, pt.y + frame0->h),
			LAYER_GUI, CAT_COMMAND, i);
	}

	// Set the cursor for when it's shown
	_vm->_events->setCursor(CURSOR_ARROW);
}

void MainMenu::doFrame() {
	// Delay between animation frames on the menu
	uint32 currTime = g_system->getMillis();
	if (currTime < _delayTimeout)
		return;
	_delayTimeout = currTime + MADS_MENU_ANIM_DELAY;

	// If an item has already been selected, handle rotating out the other menu items
	if (_selectedIndex != -1) {
		if (_frameIndex == _menuItems[0]->getCount()) {
			handleAction((MADSGameAction)_selectedIndex);
		} else {
			for (_menuItemIndex = 0; _menuItemIndex < 6; ++_menuItemIndex) {
				if (_menuItemIndex != _selectedIndex) {
					addSpriteSlot();
				}
			}

			// Move the menu items to the next frame
			++_frameIndex;
		}
		return;
	}

	// If we've alerady reached the end of the menuitem animation, exit immediately
	if (_menuItemIndex == 6)
		return;

	// If the user has chosen to skip the animation, show the full menu immediately
	if (_skipFlag && _menuItemIndex >= 0) {
		// Quickly loop through all the menu items to display each's final frame		
		for (; _menuItemIndex < 6; ++_menuItemIndex) {
			// Draw the final frame of the menuitem
			_frameIndex = 0;
			addSpriteSlot();
		}

		_vm->_events->showCursor();
	} else {
		if ((_menuItemIndex == -1) || (_frameIndex == 0)) {
			if (++_menuItemIndex == 6) {
				// Reached end of display animation
				_vm->_events->showCursor();
				return;
			}

			_frameIndex = _menuItems[_menuItemIndex]->getCount() - 1;
		} else {
			--_frameIndex;
		}

		// Move to the next menuitem frame
		addSpriteSlot();
	}
}

void MainMenu::addSpriteSlot() {
	Scene &scene = _vm->_game->_scene;
	SpriteSlots &spriteSlots = scene._spriteSlots;
	
	int seqIndex = (_menuItemIndex < 6) ? _menuItemIndex : _frameIndex;
	spriteSlots.deleteTimer(seqIndex);

	SpriteAsset *menuItem = _menuItems[_menuItemIndex];
	MSprite *spr = menuItem->getFrame(_frameIndex);

	SpriteSlot &slot = spriteSlots[spriteSlots.add()];
	slot._flags = IMG_UPDATE;
	slot._seqIndex = seqIndex;
	slot._spritesIndex = _menuItemIndexes[_menuItemIndex];
	slot._frameNumber = _frameIndex + 1;
	slot._position = spr->_offset;
	slot._depth = 1;
	slot._scale = 100;

	_redrawFlag = true;
}

bool MainMenu::onEvent(Common::Event &event) {
	Scene &scene = _vm->_game->_scene;
	if (_selectedIndex != -1)
		return false;

	// Handle keypresses - these can be done at any time, even when the menu items are being drawn
	if (event.type == Common::EVENT_KEYDOWN) {
		switch (event.kbd.keycode) {
		case Common::KEYCODE_ESCAPE:
		case Common::KEYCODE_F6:
			handleAction(EXIT);
			break;

		case Common::KEYCODE_F1:
			handleAction(START_GAME);
			break;

		case Common::KEYCODE_F2:
			handleAction(RESUME_GAME);
			break;

		case Common::KEYCODE_F3:
			handleAction(SHOW_INTRO);
			break;

		case Common::KEYCODE_F4:
			handleAction(CREDITS);
			break;

		case Common::KEYCODE_F5:
			handleAction(QUOTES);
			break;

		case Common::KEYCODE_s: {
			// Goodness knows why, but Rex has a key to restart the menuitem animations
			// Restart the animation
			_menuItemIndex = -1;
			for (int i = 0; i < 6; ++i)
				scene._spriteSlots.deleteTimer(i);

			_skipFlag = false;
			_vm->_events->hideCursor();
			break;
		}

		default:
			// Any other key skips the menu animation
			_skipFlag = true;
			return false;
		}

		return true;
	}

	switch (event.type) {
	case Common::EVENT_LBUTTONDOWN:
		if (_vm->_events->isCursorVisible()) {
			_buttonDown = true;
			int menuIndex = getHighlightedItem(event.mouse);

			if (menuIndex != _highlightedIndex) {
				scene._spriteSlots.deleteTimer(menuIndex);

				_highlightedIndex = menuIndex;
				if (_highlightedIndex != -1) {
					_frameIndex = _highlightedIndex;
					addSpriteSlot();
				}
			}
		} else {
			// Skip the menu animation
			_skipFlag = true;
		}
		return true;

	case Common::EVENT_MOUSEMOVE: 
		if (_buttonDown) {
			int menuIndex = getHighlightedItem(event.mouse);
			if (menuIndex != _highlightedIndex) {
				if (_highlightedIndex != -1) {
					// Revert to the unselected menu item
					unhighlightItem();
				}

				if (menuIndex != -1) {
					// Highlight new item
					_highlightedIndex = menuIndex;
					_frameIndex = _highlightedIndex;
					addSpriteSlot();
				}
			}
		}
		break;

	case Common::EVENT_LBUTTONUP:
		_buttonDown = false;
		if (_highlightedIndex != -1) {
			_selectedIndex = _highlightedIndex;
			unhighlightItem();
			_frameIndex = 0;
		}

		return true;

	default:
		break;
	}
	
	return false;
}

int MainMenu::getHighlightedItem(const Common::Point &pt) {
	return _vm->_game->_screenObjects.scan(pt, LAYER_GUI) - 1;
}

void MainMenu::unhighlightItem() {
	// Revert to the unselected menu item
	_vm->_game->_scene._spriteSlots.deleteTimer(_highlightedIndex);
	_menuItemIndex = _highlightedIndex;
	_frameIndex = 0;
	addSpriteSlot();

	_menuItemIndex = 6;
	_highlightedIndex = -1;
}

void MainMenu::handleAction(MADSGameAction action) {
	_vm->_events->hideCursor();
	_breakFlag = true;

	switch (action) {
	case START_GAME:
		// Show the difficulty dialog
		_vm->_dialogs->_pendingDialog = DIALOG_DIFFICULTY;
		break;

	case RESUME_GAME:
		// The original resumed the most recently saved game. Instead, 
		// just show the load game scren
		_vm->_dialogs->_pendingDialog = DIALOG_RESTORE;
		return;

	case SHOW_INTRO:
		AnimationView::execute("@rexopen");
		break;

	case CREDITS:
		TextView::execute("credits");
		return;

	case QUOTES:
		TextView::execute("quotes");
		return;

	case EXIT:
		_vm->_dialogs->_pendingDialog = DIALOG_ADVERT;
		/*
					// When the Exit action is done from the menu, show one of two possible advertisements

					// Activate the scene display with the specified scene
					bool altAdvert = _vm->_random->getRandomNumber(1000) >= 500;
					_vm->_scene->loadScene(altAdvert ? 995 : 996);
					_vm->_viewManager->addView(_vm->_scene);

					_vm->_viewManager->refreshAll();
					_vm->delay(10000);

					_vm->_events->quitFlag = true;
					return;
		*/
		break;
	default:
		break;
	}
}

/*------------------------------------------------------------------------*/

char TextView::_resourceName[100];

void TextView::execute(const Common::String &resName) {
	assert(resName.size() < 100);
	strcpy(_resourceName, resName.c_str());
}

/*------------------------------------------------------------------------*/

char AnimationView::_resourceName[100];

void AnimationView::execute(const Common::String &resName) {
	assert(resName.size() < 100);
	strcpy(_resourceName, resName.c_str());
}


} // End of namespace Nebular

} // End of namespace MADS