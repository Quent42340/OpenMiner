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
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <gk/core/input/GamePad.hpp>
#include <gk/core/ApplicationStateStack.hpp>
#include <gk/core/GameClock.hpp>
#include <gk/core/Exception.hpp>
#include <gk/core/Mouse.hpp>
#include <gk/gl/OpenGL.hpp>
#include <gk/resource/ResourceHandler.hpp>

#include "ChatState.hpp"
#include "Events.hpp"
#include "GameKey.hpp"
#include "GameState.hpp"
#include "KeyboardHandler.hpp"
#include "LuaGUIState.hpp"
#include "PauseMenuState.hpp"
#include "Registry.hpp"
#include "TextureAtlas.hpp"

GameState::GameState()
	: m_textureAtlas(gk::ResourceHandler::getInstance().get<TextureAtlas>("atlas-blocks"))
{
	Registry::setInstance(m_registry);

	initShaders();

	m_clientCommandHandler.setupCallbacks();

	m_camera.setAspectRatio((float)Config::screenWidth / Config::screenHeight);

	m_world.setClient(m_clientCommandHandler);
	m_world.setCamera(m_player.camera());
}

void GameState::init() {
	m_eventHandler->addListener<GuiScaleChangedEvent>(&GameState::onGuiScaleChanged, this);
}

void GameState::connect(const std::string &host, int port) {
	m_client.connect(host, port);
	m_player.setClientID(m_client.id());

	gk::Mouse::setCursorVisible(false);
	gk::Mouse::setCursorGrabbed(true);
}

void GameState::onEvent(const sf::Event &event) {
	if (event.type == sf::Event::Closed) {
		m_client.disconnect();

		m_stateStack->clear();
	}

	if (!m_stateStack->empty() && &m_stateStack->top() == this) {
		KeyboardHandler *keyboardHandler = (KeyboardHandler *)gk::GamePad::getInputHandler();

#ifdef __APPLE__
		if (event.type == sf::Event::MouseMoved) {
			gk::Mouse::update(event);

			const auto &delta = gk::Mouse::getDelta();
			m_player.turnH(delta.x * -0.01 * Config::mouseSensitivity);
			m_player.turnViewV(delta.y * -0.01 * Config::mouseSensitivity);
		}
#else
		if (event.type == sf::Event::MouseMovedRaw) {
			m_player.turnH(event.mouseMoveRaw.deltaX * -0.01 * Config::mouseSensitivity);
			m_player.turnViewV(event.mouseMoveRaw.deltaY * -0.01 * Config::mouseSensitivity);

			gk::Mouse::resetToWindowCenter();
		}
#endif
		else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
			m_stateStack->push<PauseMenuState>(m_client, this);
		}
		else if (event.type == sf::Event::KeyPressed
		&& (event.key.code == keyboardHandler->getKeycode(GameKey::Chat)
		 || event.key.code == keyboardHandler->getKeycode(GameKey::Command)))
		{
			m_stateStack->push<ChatState>(m_clientCommandHandler, m_hud.chat(), event.key.code == keyboardHandler->getKeycode(GameKey::Command), this);
		}
		else if (event.type == sf::Event::LostFocus) {
			m_stateStack->push<PauseMenuState>(m_client, this);

			gk::Mouse::setCursorGrabbed(false);
			gk::Mouse::setCursorVisible(true);
		}
		else if (event.type == sf::Event::GainedFocus) {
			gk::Mouse::setCursorGrabbed(true);
			gk::Mouse::setCursorVisible(false);
		}

		m_hud.onEvent(event);
	}

	if (event.type == sf::Event::Resized) {
		Config::screenWidth = event.size.width;
		Config::screenHeight = event.size.height;

		m_camera.setAspectRatio((float)Config::screenWidth / Config::screenHeight);
		m_hud.setup();
	}
}

void GameState::update() {
	// m_world.checkPlayerChunk(m_player.x(), m_player.y(), m_player.z());
	m_world.update();

	if (m_camera.getFieldOfView() != Config::cameraFOV)
		m_camera.setFieldOfView(Config::cameraFOV);

	if (!m_stateStack->empty() && &m_stateStack->top() == this) {
		m_player.processInputs();

		if (gk::GamePad::isKeyPressedOnce(GameKey::Inventory)) {
			m_clientCommandHandler.sendPlayerInventoryRequest();
		}
		else if (gk::GamePad::isKeyPressedOnce(GameKey::CreativeWindow)) {
			m_clientCommandHandler.sendPlayerCreativeWindowRequest();
		}
	}

	m_player.updatePosition(m_world);

	m_hud.update();

	if (gk::GameClock::getInstance().getTicks() % 100 < 10) {
		m_clientCommandHandler.sendPlayerPosUpdate();
		m_clientCommandHandler.sendPlayerRotUpdate();
	}

	m_client.update();
}

void GameState::initShaders() {
	m_shader.createProgram();

	m_shader.addShader(GL_VERTEX_SHADER, "resources/shaders/game.v.glsl");
	m_shader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/light.f.glsl");
	m_shader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/fog.f.glsl");
	m_shader.addShader(GL_FRAGMENT_SHADER, "resources/shaders/game.f.glsl");

	m_shader.linkProgram();
}

void GameState::onGuiScaleChanged(const GuiScaleChangedEvent &event) {
	m_hud.onGuiScaleChanged(event);
}

void GameState::draw(gk::RenderTarget &target, gk::RenderStates states) const {
	// FIXME: This uniform is not used anymore since water/leaves effects are disabled
	// gk::Shader::bind(&m_shader);
	// m_shader.setUniform("u_time", gk::GameClock::getInstance().getTicks());
	// gk::Shader::bind(nullptr);

	states.shader = &m_shader;

	target.setView(m_camera);
	target.draw(m_world, states);

	for (auto &it : m_playerBoxes)
		if (it.second.dimension() == m_player.dimension())
			target.draw(it.second, states);

	target.draw(m_hud, states);
}

