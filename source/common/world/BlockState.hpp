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
#ifndef BLOCKSTATE_HPP_
#define BLOCKSTATE_HPP_

#include <array>
#include <string>
#include <unordered_map>

#include <gk/core/Box.hpp>
#include <gk/core/IntTypes.hpp>

#include "BlockParam.hpp"
#include "ItemStack.hpp"
#include "TilesDef.hpp"

enum class BlockDrawType : u8 {
	Solid       = 0,
	XShape      = 1,
	Leaves      = 2,
	Liquid      = 3,
	Glass       = 4,
	Cactus      = 5,
	SubBoxes    = 6,
};

#define BLOCK_ATTR_GETTER(attrName) \
	auto attrName() const -> const std::remove_reference<decltype(m_##attrName)>::type & { \
		if (!m_defaultState || m_attrs & BlockAttribute::attr_##attrName) { \
			return m_##attrName; \
		} \
		else { \
			return m_defaultState->m_##attrName; \
		} \
	}

#define BLOCK_ATTR_SETTER(attrName) \
	void attrName(const std::remove_reference<decltype(m_##attrName)>::type &value) { \
		m_##attrName = value; \
		m_attrs |= BlockAttribute::attr_##attrName; \
	}

#define BLOCK_ATTR(type, name) \
	private: \
		type m_##name; \
	public: \
		BLOCK_ATTR_GETTER(name) \
		BLOCK_ATTR_SETTER(name)

#define BLOCK_ATTR_V(type, name, defaultValue) \
	private: \
		type m_##name = defaultValue; \
	public: \
		BLOCK_ATTR_GETTER(name) \
		BLOCK_ATTR_SETTER(name)


class BlockState : public gk::ISerializable {
	public:
		BlockState() = default;
		BlockState(u16 id, const Block *block, const BlockState *defaultState)
			: m_block(block), m_defaultState(defaultState), m_id(id) {}

		void serialize(sf::Packet &packet) const override;
		void deserialize(sf::Packet &packet) override;

		const Block &block() const { return *m_block; }

		u16 id() const { return m_id; }

		float timeToBreak(u8 harvestCapability, float miningSpeed, bool isEffective) const {
			if (isEffective || (harvestRequirements() & harvestCapability) || (harvestCapability == 0 && harvestRequirements() == 0))
				return 1.5 * m_hardness / miningSpeed;
			else
				return 5 * m_hardness;
		}

		ItemStack getItemDrop() const { return {m_itemDrop, m_itemDropAmount}; }

		void setBlock(const Block *block) { m_block = block; }
		void setDefaultState(const BlockState *defaultState) { m_defaultState = defaultState; }

		enum SubBoxType : u8 {
			Fixed       = 0,
			Connected   = 1,
			WallMounted = 2,
		};

		enum WallMountedBoxType : u8 {
			WallTop    = 0,
			WallBottom = 1,
			WallSides  = 2,
		};

		void addSubBox(const gk::FloatBox &subBox) { m_subBoxes.emplace_back(subBox); }
		void addConnectedSubBox(BlockFace face, const gk::FloatBox &subBox) { m_connectedSubBoxes[face].emplace_back(subBox); }
		void addWallMountedBox(WallMountedBoxType type, const gk::FloatBox &subBox) { m_connectedSubBoxes[type + 6].emplace_back(subBox); }
		const std::vector<gk::FloatBox> &wallMountedBoxes(WallMountedBoxType type) const { return m_connectedSubBoxes[type + 6]; }

		static void initUsertype(sol::state &lua);

	private:
		const Block *m_block = nullptr;
		const BlockState *m_defaultState = nullptr;

		u16 m_id = 0;

		enum BlockAttribute : u32 {
			attr_label                = 1 << 0,
			attr_tiles                = 1 << 1,
			attr_itemDrop             = 1 << 2,
			attr_itemDropAmount       = 1 << 3,
			attr_harvestRequirements  = 1 << 4,
			attr_hardness             = 1 << 5,
			attr_boundingBox          = 1 << 6,
			attr_drawType             = 1 << 7,
			attr_colorMultiplier      = 1 << 8,
			attr_isOpaque             = 1 << 9,
			attr_isLightSource        = 1 << 10,
			attr_inventoryImage       = 1 << 11,
			attr_fogDepth             = 1 << 12,
			attr_fogColor             = 1 << 13,
			attr_drawOffset           = 1 << 14,
			attr_isCollidable         = 1 << 15,
			attr_subBoxesType         = 1 << 16,
			attr_subBoxes             = 1 << 17,
			attr_connectedSubBoxes    = 1 << 18,
		};

		BLOCK_ATTR(std::string, label);
		BLOCK_ATTR(TilesDef, tiles);

		BLOCK_ATTR(std::string, itemDrop);
		BLOCK_ATTR_V(u16, itemDropAmount, 1);

		BLOCK_ATTR_V(u8, harvestRequirements, 0);
		BLOCK_ATTR_V(float, hardness, 1.0f);

		BLOCK_ATTR_V(gk::FloatBox, boundingBox, (gk::FloatBox{0, 0, 0, 1, 1, 1}));

		BLOCK_ATTR_V(BlockDrawType, drawType, BlockDrawType::Solid);

		BLOCK_ATTR_V(gk::Color, colorMultiplier, gk::Color::White);

		BLOCK_ATTR_V(bool, isLightSource, false);

		BLOCK_ATTR(std::string, inventoryImage);

		BLOCK_ATTR_V(float, fogDepth, 0);
		BLOCK_ATTR_V(gk::Color, fogColor, gk::Color::White);

		BLOCK_ATTR_V(gk::Vector3f, drawOffset, (gk::Vector3f{0, 0, 0}));

		BLOCK_ATTR_V(bool, isCollidable, true);

		using ConnectedSubBoxes = std::array<std::vector<gk::FloatBox>, 9>;
		BLOCK_ATTR(SubBoxType, subBoxesType);
		BLOCK_ATTR(std::vector<gk::FloatBox>, subBoxes);
		BLOCK_ATTR(ConnectedSubBoxes, connectedSubBoxes);

		u32 m_attrs = 0;

	// isOpaque needs a custom getter
	private:
		bool m_isOpaque = true;
	public:
		bool isOpaque() const;
		BLOCK_ATTR_SETTER(isOpaque);
};

#undef BLOCK_ATTR
#undef BLOCK_ATTR_GETTER
#undef BLOCK_ATTR_SETTER

#endif // BLOCKSTATE_HPP_
