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
#include "BlockState.hpp"
#include "Registry.hpp"

void BlockState::serialize(sf::Packet &packet) const {
	packet << m_id << m_label << m_tiles
		<< m_itemDrop << m_itemDropAmount
		<< m_hardness << m_harvestRequirements
		<< m_boundingBox << u8(m_drawType) << m_colorMultiplier
		<< m_isOpaque << m_isLightSource
		<< m_inventoryImage << m_fogDepth << m_fogColor
		<< m_drawOffset << m_isCollidable << m_attrs << m_subBoxes;
}

void BlockState::deserialize(sf::Packet &packet) {
	u8 drawType;

	packet >> m_id >> m_label >> m_tiles
		>> m_itemDrop >> m_itemDropAmount
		>> m_hardness >> m_harvestRequirements
		>> m_boundingBox >> drawType >> m_colorMultiplier
		>> m_isOpaque >> m_isLightSource
		>> m_inventoryImage >> m_fogDepth >> m_fogColor
		>> m_drawOffset >> m_isCollidable >> m_attrs >> m_subBoxes;

	m_drawType = BlockDrawType(drawType);
}

bool BlockState::isOpaque() const {
	bool isOpaque = (!m_defaultState || m_attrs & BlockAttribute::attr_isOpaque) ? m_isOpaque : m_defaultState->m_isOpaque;
	return m_block && m_block->id() != 0 && isOpaque && drawType() != BlockDrawType::XShape;
}

// Please update 'docs/lua-api-cpp.md' if you change this
void BlockState::initUsertype(sol::state &lua) {
	lua.new_usertype<BlockState>("BlockState",
		"id", &BlockState::id,
		"label", (const std::string &(BlockState::*)() const)&BlockState::label,
		"get_item_drop", &BlockState::getItemDrop
	);
}

