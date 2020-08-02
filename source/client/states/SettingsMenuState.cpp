/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

#include <gk/core/input/GamePad.hpp>
#include <gk/core/ApplicationStateStack.hpp>
#include <gk/core/Debug.hpp>
#include <gk/core/EventHandler.hpp>
#include <gk/core/Mouse.hpp>
#include <gk/core/Utils.hpp>

#include "Config.hpp"
#include "Events.hpp"
#include "KeyboardHandler.hpp"
#include "Registry.hpp"
#include "SettingsMenuState.hpp"
#include "TexturePackSelectionState.hpp"
#include "World.hpp"

SettingsMenuState::SettingsMenuState(gk::ApplicationState *parent) : InterfaceState(parent) {
	m_background.setScale(Config::guiScale * 2, Config::guiScale * 2);

	m_filter1.setFillColor(gk::Color(0, 0, 0, 192));
	m_filter2.setFillColor(gk::Color(0, 0, 0, 120));

	m_title.setScale(Config::guiScale, Config::guiScale);
	m_title.setString("Options");
	m_title.updateVertexBuffer();

	m_menuWidget.setScale(Config::guiScale, Config::guiScale);

	m_doneButton.setScale(Config::guiScale, Config::guiScale);
	m_doneButton.setText("Done");
	m_doneButton.setCallback([this] (TextButton &) {
		doneButtonAction();
	});

	updateWidgetPosition();

	addMainButtons();
}

void SettingsMenuState::init() {
	m_eventHandler->addListener<GuiScaleChangedEvent>(&SettingsMenuState::onGuiScaleChanged, this);
}

void SettingsMenuState::onEvent(const SDL_Event &event) {
	InterfaceState::onEvent(event);

	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if (&m_stateStack->top() != this)
			m_menuWidget.onEvent(event);
		else if (m_state == MenuState::Graphics && m_resolutionButton)
			m_resolutionButton->setText("Resolution: " + std::to_string(Config::screenWidth) + "x" + std::to_string(Config::screenHeight));
	}

	if (!m_stateStack->empty() && &m_stateStack->top() == this) {
		m_menuWidget.onEvent(event);
		m_doneButton.onEvent(event);

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			doneButtonAction();
		}
		else if (m_currentKeyButton && event.type == SDL_KEYDOWN) {
			KeyboardHandler *keyboardHandler = dynamic_cast<KeyboardHandler *>(gk::GamePad::getInputHandler());
			keyboardHandler->setKeycode(m_currentKey, event.key.keysym.sym);

			m_key->setKeycode(event.key.keysym.sym);
			m_key = nullptr;

			m_currentKeyButton->setText(m_currentKeyButton->text() + keyboardHandler->getKeyName(m_currentKey));
			m_currentKeyButton = nullptr;
		}
	}
}

void SettingsMenuState::onGuiScaleChanged(const GuiScaleChangedEvent &event) {
	m_background.setScale(event.guiScale * 2, event.guiScale * 2);
	m_title.setScale(event.guiScale, event.guiScale);

	m_menuWidget.setScale(event.guiScale, event.guiScale);
	m_doneButton.setScale(event.guiScale, event.guiScale);

	updateWidgetPosition();

	m_menuWidget.onGuiScaleChanged(event);
}

void SettingsMenuState::updateWidgetPosition() {
	m_background.setPosRect(0, 0, Config::screenWidth / m_background.getScale().x, Config::screenHeight / m_background.getScale().y);
	m_background.setClipRect(0, 0, Config::screenWidth / m_background.getScale().x, Config::screenHeight / m_background.getScale().y);

	m_filter1.setSize(Config::screenWidth, Config::screenHeight);

	const int topBorderSize = 25 * Config::guiScale;
	const int bottomBorderSize = 25 * Config::guiScale;
	m_filter2.setSize(Config::screenWidth, Config::screenHeight - topBorderSize - bottomBorderSize);
	m_filter2.setPosition(0, topBorderSize);

	m_title.setPosition(
		roundf(Config::screenWidth / 2.0f - m_title.getSize().x * Config::guiScale / 2.0f),
		roundf(topBorderSize / 2.0f - m_title.getSize().y * Config::guiScale / 2.0f)
	);

	m_doneButton.setPosition(
		roundf(Config::screenWidth / 2.0f - m_doneButton.getGlobalBounds().sizeX / 2.0f),
		roundf(Config::screenHeight - bottomBorderSize / 2.0f - m_doneButton.getGlobalBounds().sizeY / 2.0f)
	);

	m_menuWidget.setPosition(
		roundf(Config::screenWidth / 2.0f - m_menuWidget.getGlobalBounds().sizeX / 2.0f),
		roundf(topBorderSize + 5.0f * Config::guiScale)
	);
}

void SettingsMenuState::doneButtonAction() {
	if (m_state != MenuState::Main) {
		m_resolutionButton = nullptr;
		m_state = MenuState::Main;
		addMainButtons();
	} else {
		m_stateStack->pop();
	}
}

void SettingsMenuState::addMainButtons() {
	m_menuWidget.reset(1, 8);

	m_menuWidget.addButton("Gameplay...", [this] (TextButton &) {
		m_state = MenuState::Gameplay;
		addGameplayButtons();
	});

	m_menuWidget.addButton("Interface...", [this] (TextButton &) {
		m_state = MenuState::Gameplay;
		addInterfaceButtons();
	});

	m_menuWidget.addButton("Graphics...", [this] (TextButton &) {
		m_state = MenuState::Graphics;
		addGraphicsButtons();
	});

	m_menuWidget.addButton("Input...", [this] (TextButton &) {
		m_state = MenuState::Input;
		addInputButtons();
	});

	m_menuWidget.addButton("Debug...", [this] (TextButton &) {
		m_state = MenuState::Debug;
		addDebugButtons();
	});

	m_menuWidget.addButton("Texture Pack...", [this] (TextButton &) {
		m_stateStack->push<TexturePackSelectionState>(this);
	});

	updateWidgetPosition();
}

void SettingsMenuState::addGameplayButtons() {
	m_menuWidget.reset(1, 8);

	addToggleButton("No Clip", Config::isNoClipEnabled, false);
	addToggleButton("Fly Mode", Config::isFlyModeEnabled, false);

	m_menuWidget.addSlider("Max Block Reach: " + std::to_string(Config::maxBlockReach), [] (SliderWidget &slider, u32) {
		Config::maxBlockReach = slider.getCurrentValue();
		slider.setText("Max Block Reach: " + std::to_string(Config::maxBlockReach));
	}, 1, 15, Config::maxBlockReach);

	updateWidgetPosition();
}

void SettingsMenuState::addInterfaceButtons() {
	m_menuWidget.reset(1, 8);

	addToggleButton("Show block info", Config::isBlockInfoWidgetEnabled, false);
	addToggleButton("Show FPS counter", Config::isFpsCounterEnabled, false);
	addToggleButton("Show hotbar", Config::isHotbarVisible, false);
	addToggleButton("Show crosshair", Config::isCrosshairVisible, false);

	updateWidgetPosition();
}

void SettingsMenuState::addGraphicsButtons() {
	m_menuWidget.reset(2, 8);

	m_menuWidget.addSlider("Render Distance: " + std::to_string(Config::renderDistance), [] (SliderWidget &slider, u32) {
		Config::renderDistance = slider.getCurrentValue();
		slider.setText("Render Distance: " + std::to_string(Config::renderDistance));
		World::isReloadRequested = true;
	}, 4, 16, Config::renderDistance);

	addToggleButton("Wireframe Mode", Config::isWireframeModeEnabled, false);

	m_menuWidget.addButton(std::string("Smooth Lighting: ") + (Config::isSmoothLightingEnabled ? "ON" : "OFF"), [&] (TextButton &button) {
		Config::isSmoothLightingEnabled = !Config::isSmoothLightingEnabled;
		button.setText(std::string("Smooth Lighting: ") + (Config::isSmoothLightingEnabled ? "ON" : "OFF"));

		World::isReloadRequested = true;
	});

	const std::string aoValueNames[3] = {
		"OFF",
		"Fast",
		"Fancy"
	};

	m_menuWidget.addButton(std::string("Ambient Occlusion: ") + aoValueNames[Config::ambientOcclusion], [&, aoValueNames] (TextButton &button) {
		Config::ambientOcclusion = (Config::ambientOcclusion + 1) % (Config::isSmoothLightingEnabled ? 3 : 2);
		button.setText(std::string("Ambient Occlusion: ") + aoValueNames[Config::ambientOcclusion]);

		World::isReloadRequested = true;
	});

	m_menuWidget.addSlider("GUI Scale: " + std::to_string(Config::guiScale), [this] (SliderWidget &slider, u32 eventType) {
		slider.setText("GUI Scale: " + std::to_string(slider.getCurrentValue()));
		if (eventType == SDL_MOUSEBUTTONUP) {
			Config::guiScale = slider.getCurrentValue();
			m_eventHandler->emplaceEvent<GuiScaleChangedEvent>(Config::guiScale);
		}
	}, 1, 3, Config::guiScale);

	m_resolutionButton = &m_menuWidget.addButton("Resolution: " + std::to_string(Config::screenWidth) + "x" + std::to_string(Config::screenHeight), [] (TextButton &button) {
		if (Config::isFullscreenModeEnabled) return;

		// FIXME: Find a better way to do this
		if (Config::screenWidth == 1600 && Config::screenHeight == 1050) {
			Config::screenWidth = 1280;
			Config::screenHeight = 720;
		}
		else if (Config::screenWidth == 1280 && Config::screenHeight == 720) {
			Config::screenWidth = 1920;
			Config::screenHeight = 1080;
		}
		else {
			Config::screenWidth = 1600;
			Config::screenHeight = 1050;
		}

		button.setText("Resolution: " + std::to_string(Config::screenWidth) + "x" + std::to_string(Config::screenHeight));
	});

	m_resolutionButton->setEnabled(!Config::isFullscreenModeEnabled);

	m_menuWidget.addButton(std::string("Fullscreen: ") + (Config::isFullscreenModeEnabled ? "ON" : "OFF"), [&, aoValueNames] (TextButton &button) {
		Config::isFullscreenModeEnabled = !Config::isFullscreenModeEnabled;
		button.setText(std::string("Fullscreen: ") + (Config::isFullscreenModeEnabled ? "ON" : "OFF"));

		m_resolutionButton->setEnabled(!Config::isFullscreenModeEnabled);
	});

	addToggleButton("Use VSync", Config::isVerticalSyncEnabled, false);

	m_menuWidget.addSlider("Mipmap Levels: " + std::to_string(Config::mipmapLevels), [] (SliderWidget &slider, u32) {
		Config::mipmapLevels = slider.getCurrentValue();
		slider.setText("Mipmap Levels: " + std::to_string(Config::mipmapLevels));
	}, 0, 4, Config::mipmapLevels);

	addToggleButton("Star Rendering", Config::isStarRenderingEnabled, false);

	m_menuWidget.addSlider("FOV: " + std::to_string((int)Config::cameraFOV), [] (SliderWidget &slider, u32) {
		Config::cameraFOV = slider.getCurrentValue();
		slider.setText("FOV: " + std::to_string((int)Config::cameraFOV));
	}, 45, 135, Config::cameraFOV);

	updateWidgetPosition();
}

void SettingsMenuState::addInputButtons() {
	KeyboardHandler *keyboardHandler = dynamic_cast<KeyboardHandler *>(gk::GamePad::getInputHandler());
	m_menuWidget.reset(2, keyboardHandler->keyCount() / 2.f + 1.5f);

	for (auto &it : keyboardHandler->keys()) {
		m_menuWidget.addButton(it.second.name() + ": " + keyboardHandler->getKeyName(it.first), [this, it] (TextButton &button) {
			button.setText(it.second.name() + ": ");
			m_currentKey = it.first;
			m_currentKeyButton = &button;
			m_key = const_cast<Key *>(&it.second);
		});
	}

	m_menuWidget.addSlider("Mouse Sensitivity: " + std::to_string(Config::mouseSensitivity), [] (SliderWidget &slider, u32) {
		Config::mouseSensitivity = slider.getCurrentValue();
		slider.setText("Mouse Sensitivity: " + std::to_string(Config::mouseSensitivity));
	}, 4, 32, Config::mouseSensitivity);

	updateWidgetPosition();
}

void SettingsMenuState::addDebugButtons() {
	m_menuWidget.reset(1, 8);

	addToggleButton("Show chunk minimap", Config::isChunkMinimapEnabled, false);

	updateWidgetPosition();
}

TextButton &SettingsMenuState::addToggleButton(const std::string &text, bool &configOption, bool worldReloadRequested) {
	return m_menuWidget.addButton(text + ": " + (configOption ? "ON" : "OFF"), [=, &configOption] (TextButton &button) {
		configOption = !configOption;
		button.setText(text + ": " + (configOption ? "ON" : "OFF"));

		if (worldReloadRequested)
			World::isReloadRequested = true;
	});
}

void SettingsMenuState::draw(gk::RenderTarget &target, gk::RenderStates states) const {
	prepareDraw(target, states);

	target.draw(m_background, states);
	target.draw(m_filter1, states);
	target.draw(m_filter2, states);

	target.draw(m_title, states);

	target.draw(m_menuWidget, states);
	target.draw(m_doneButton, states);

	target.disableView();
}

