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
#include <gk/core/Debug.hpp>

#include "BlockPlacementConstraints.hpp"
#include "LuaBlockLoader.hpp"
#include "LuaMod.hpp"
#include "Registry.hpp"
#include "ServerBlock.hpp"
#include "ServerConfig.hpp"

void LuaBlockLoader::loadBlock(const sol::table &table) const {
	std::string stringID = m_mod.id() + ":" + table["id"].get<std::string>();

	ServerBlock &block = Registry::getInstance().registerBlock<ServerBlock>(stringID);
	block.setRotatable(table["is_rotatable"].get_or(false));

	block.setOnBlockActivated(table["on_block_activated"]);
	block.setOnTick(table["on_tick"]);
	block.setOnBlockPlaced(table["on_block_placed"]);
	block.setOnBlockDestroyed(table["on_block_destroyed"]);
	block.setTickRandomly(table["tick_randomly"].get_or(false));
	block.setTickProbability(table["tick_probability"].get_or(0.f));
	block.setCustomParamBits(table["custom_param_bits"].get_or<u8>(0));

	BlockState &defaultState = block.getState(0);
	loadBlockState(defaultState, table, block);

	Item *item = nullptr;
	if (!defaultState.inventoryImage().empty()) {
		TilesDef tilesDef{block.stringID(), 0, defaultState.inventoryImage()};
		item = &Registry::getInstance().registerItem<Item>(tilesDef, block.stringID(), defaultState.label());
	}
	else {
		item = &Registry::getInstance().registerItem<Item>(defaultState.tiles(), block.stringID(), defaultState.label());
	}

	item->setIsBlock(true);
	item->setMaxStackSize(table["max_stack_size"].get_or(ServerConfig::maxItemStackSize));

	loadGroups(block, table, item);

	loadParams(block);
}

void LuaBlockLoader::loadBlockState(BlockState &state, const sol::table &table, ServerBlock &block) const {
	TilesDef tiles{block.stringID(), state.id()};
	tiles.loadFromLuaTable(table);
	if (!tiles.textureFilenames().empty())
		state.tiles(tiles);

	loadDrawType(state, table, block);
	if (state.drawType() == BlockDrawType::SubBoxes)
		loadSubBoxes(state, table);

	loadProperties(state, table);
	loadBoundingBox(state, table);
	loadItemDrop(state, table);
	loadColorMultiplier(state, table);
	loadPlacementConstraints(block, table);

	loadStates(block, state, table);
}

inline void LuaBlockLoader::loadProperties(BlockState &state, const sol::table &table) const {
	if (table["name"].get_type() == sol::type::string)
		state.label(table["name"].get<std::string>());

	if (table["harvest_requirements"].get_type() == sol::type::number)
		state.harvestRequirements(table["harvest_requirements"].get<int>());

	if (table["hardness"].get_type() == sol::type::number)
		state.hardness(table["hardness"].get<float>());

	if (table["is_opaque"].get_type() == sol::type::boolean)
		state.isOpaque(table["is_opaque"].get<bool>());

	if (table["is_light_source"].get_type() == sol::type::boolean)
		state.isLightSource(table["is_light_source"].get<bool>());

	if (table["inventory_image"].get_type() == sol::type::string)
		state.inventoryImage(table["inventory_image"].get<std::string>());

	if (table["fog_depth"].get_type() == sol::type::number)
		state.fogDepth(table["fog_depth"].get<float>());

	if (state.fogDepth()) {
		sol::optional<sol::table> fogColor = table["fog_color"];
		if (fogColor != sol::nullopt) {
			state.fogColor(gk::Color{
				fogColor.value().get<u8>(1),
				fogColor.value().get<u8>(2),
				fogColor.value().get<u8>(3),
			});
		}
	}

	sol::optional<sol::table> drawOffset = table["draw_offset"];
	if (drawOffset != sol::nullopt) {
		state.drawOffset(gk::Vector3f{
			drawOffset.value().get<float>(1),
			drawOffset.value().get<float>(2),
			drawOffset.value().get<float>(3),
		});
	}

	if (table["is_collidable"].get_type() == sol::type::boolean)
		state.isCollidable(table["is_collidable"].get<bool>());
}

inline void LuaBlockLoader::loadSubBoxes(BlockState &state, const sol::table &table) const {
	sol::optional<sol::table> subBoxes = table["subboxes"];
	if (subBoxes != sol::nullopt) {
		static std::unordered_map<std::string, BlockState::SubBoxType> typeMap{
			{"fixed",       BlockState::Fixed},
			{"connected",   BlockState::Connected},
			{"wallmounted", BlockState::WallMounted},
		};

		sol::table subBoxesTable = subBoxes.value();
		std::string type = subBoxesTable["type"].get<std::string>();

		auto it = typeMap.find(type);
		if (it != typeMap.end()) {
			state.subBoxesType(it->second);
		}
		else {
			gkError() << ("For block '" + state.block().stringID() + "': subboxes type '" + type + "' doesn't exist").c_str();
			return;
		}

		if (state.subBoxesType() == BlockState::Fixed || state.subBoxesType() == BlockState::Connected) {
			sol::table fixedTable = subBoxesTable["fixed"];
			for (auto &it : fixedTable) {
				if (it.second.get_type() == sol::type::table) {
					state.addSubBox(gk::FloatBox{
						it.second.as<sol::table>().get<float>(1),
						it.second.as<sol::table>().get<float>(2),
						it.second.as<sol::table>().get<float>(3),
						it.second.as<sol::table>().get<float>(4),
						it.second.as<sol::table>().get<float>(5),
						it.second.as<sol::table>().get<float>(6),
					});
				}
				else if (it.second.get_type() == sol::type::number) {
					state.addSubBox(gk::FloatBox{
						fixedTable.get<float>(1),
						fixedTable.get<float>(2),
						fixedTable.get<float>(3),
						fixedTable.get<float>(4),
						fixedTable.get<float>(5),
						fixedTable.get<float>(6),
					});
					break;
				}
			}
		}

		if (state.subBoxesType() == BlockState::Connected) {
			const char *names[6] = {
				"connect_west",
				"connect_east",
				"connect_south",
				"connect_north",
				"connect_bottom",
				"connect_top",
			};

			for (int i = 0 ; i < 6 ; ++i) {
				sol::optional<sol::table> connectObject = subBoxesTable[names[i]];
				if (connectObject != sol::nullopt) {
					for (auto &it : connectObject.value()) {
						if (it.second.get_type() == sol::type::table) {
							state.addConnectedSubBox((BlockFace)i, gk::FloatBox{
								it.second.as<sol::table>().get<float>(1),
								it.second.as<sol::table>().get<float>(2),
								it.second.as<sol::table>().get<float>(3),
								it.second.as<sol::table>().get<float>(4),
								it.second.as<sol::table>().get<float>(5),
								it.second.as<sol::table>().get<float>(6),
							});
						}
						else if (it.second.get_type() == sol::type::number) {
							state.addConnectedSubBox((BlockFace)i, gk::FloatBox{
								connectObject.value().get<float>(1),
								connectObject.value().get<float>(2),
								connectObject.value().get<float>(3),
								connectObject.value().get<float>(4),
								connectObject.value().get<float>(5),
								connectObject.value().get<float>(6),
							});
							break;
						}
					}
				}
			}
		}

		if (state.subBoxesType() == BlockState::WallMounted) {
			const char *names[3] = {
				"wall_top",
				"wall_bottom",
				"wall_side",
			};

			for (int i = 0 ; i < 3 ; ++i) {
				sol::optional<sol::table> wallObject = subBoxesTable[names[i]];
				if (wallObject != sol::nullopt) {
					for (auto &it : wallObject.value()) {
						if (it.second.get_type() == sol::type::table) {
							state.addWallMountedBox((BlockState::WallMountedBoxType)i, gk::FloatBox{
								it.second.as<sol::table>().get<float>(1),
								it.second.as<sol::table>().get<float>(2),
								it.second.as<sol::table>().get<float>(3),
								it.second.as<sol::table>().get<float>(4),
								it.second.as<sol::table>().get<float>(5),
								it.second.as<sol::table>().get<float>(6),
							});
						}
						else if (it.second.get_type() == sol::type::number) {
							state.addWallMountedBox((BlockState::WallMountedBoxType)i, gk::FloatBox{
								wallObject.value().get<float>(1),
								wallObject.value().get<float>(2),
								wallObject.value().get<float>(3),
								wallObject.value().get<float>(4),
								wallObject.value().get<float>(5),
								wallObject.value().get<float>(6),
							});
							break;
						}
					}
				}
			}
		}
	}
}

inline void LuaBlockLoader::loadBoundingBox(BlockState &state, const sol::table &table) const {
	sol::optional<sol::table> boundingBox = table["bounding_box"];
	if (boundingBox != sol::nullopt) {
		state.boundingBox(gk::FloatBox{
			boundingBox.value().get<float>(1),
			boundingBox.value().get<float>(2),
			boundingBox.value().get<float>(3),
			boundingBox.value().get<float>(4),
			boundingBox.value().get<float>(5),
			boundingBox.value().get<float>(6),
		});
	}
}

inline void LuaBlockLoader::loadDrawType(BlockState &state, const sol::table &table, const ServerBlock &block) const {
	sol::object drawTypeObject = table["draw_type"];
	if (drawTypeObject.valid()) {
		if (drawTypeObject.get_type() == sol::type::string) {
			static const std::unordered_map<std::string, BlockDrawType> drawTypes = {
				{"solid",       BlockDrawType::Solid},
				{"xshape",      BlockDrawType::XShape},
				{"leaves",      BlockDrawType::Leaves},
				{"liquid",      BlockDrawType::Liquid},
				{"glass",       BlockDrawType::Glass},
				{"cactus",      BlockDrawType::Cactus},
				{"subboxes",    BlockDrawType::SubBoxes},
			};

			auto it = drawTypes.find(drawTypeObject.as<std::string>());
			if (it != drawTypes.end()) {
				state.drawType(it->second);
			}
			else
				gkError() << "In" << block.stringID() << " definition: Block draw type invalid";
		}
		else
			gkError() << "In" << block.stringID() << " definition: Block draw type must be a string";
	}

	if (state.drawType() == BlockDrawType::Liquid || state.drawType() == BlockDrawType::XShape) {
		state.isCollidable(false);
	}
}

inline void LuaBlockLoader::loadItemDrop(BlockState &state, const sol::table &table) const {
	sol::optional<sol::table> itemDrop = table["item_drop"];
	if (itemDrop != sol::nullopt) {
		state.itemDrop(itemDrop.value()["id"]);
		state.itemDropAmount(itemDrop.value()["amount"]);
	}
}

inline void LuaBlockLoader::loadColorMultiplier(BlockState &state, const sol::table &table) const {
	sol::optional<sol::table> colorMultiplier = table["color_multiplier"];
	if (colorMultiplier != sol::nullopt) {
		state.colorMultiplier(gk::Color{
			colorMultiplier.value().get<u8>(1),
			colorMultiplier.value().get<u8>(2),
			colorMultiplier.value().get<u8>(3),
			colorMultiplier.value().get<u8>(4)
		});
	}
}

inline void LuaBlockLoader::loadStates(ServerBlock &block, BlockState &state, const sol::table &table) const {
	sol::object statesObject = table["states"];
	if (statesObject.valid()) {
		if (statesObject.get_type() == sol::type::table) {
			sol::table statesTable = statesObject.as<sol::table>();
			for (auto &statesObject : statesTable) {
				unsigned int stateID = statesObject.first.as<unsigned int>();
				if (stateID == block.states().size()) {
					BlockState &state = block.addState();
					loadBlockState(state, statesObject.second.as<sol::table>(), block);
				}
				else {
					gkError() << ("For block '" + block.stringID() + "':").c_str() << "States must be defined in a correct order starting from 1";
					gkError() << "StateID:" << stateID << "| States registered:" << block.states().size();
				}
			}
		}
		else
			gkError() << "For block" << state.block().stringID() << ": 'states' must be a table";
	}
}

inline void LuaBlockLoader::loadPlacementConstraints(ServerBlock &block, const sol::table &table) const {
	sol::object constraintsObject = table["placement_constraints"];
	if (constraintsObject.valid()) {
		if (constraintsObject.get_type() == sol::type::table) {
			sol::table constraintsTable = constraintsObject.as<sol::table>();
			for (auto &constraintsObject : constraintsTable) {
				BlockPlacementConstraint constraint;

				sol::optional<sol::table> blockOffset = constraintsObject.first.as<sol::table>();
				if (blockOffset != sol::nullopt) {
					constraint.blockOffset.x = blockOffset.value().get<u32>(1);
					constraint.blockOffset.y = blockOffset.value().get<u32>(2);
					constraint.blockOffset.z = blockOffset.value().get<u32>(3);
				}
				else
					gkError() << "For block" << block.stringID() << ": 'placement_constraints' offset is wrong";

				sol::optional<sol::table> constraintTable = constraintsObject.second.as<sol::table>();
				if (constraintTable != sol::nullopt) {
					constraint.blockID = constraintTable.value()["block"].get<std::string>();
					constraint.isWhitelist = constraintTable.value()["is_whitelist"].get<bool>();
				}
				else
					gkError() << "For block" << block.stringID() << ": 'placement_constraints' table is wrong";

				block.placementConstraints().addConstraint(constraint);
			}
		}
		else
			gkError() << "For block" << block.stringID() << ": 'placement_constraints' must be a table";
	}
}

inline void LuaBlockLoader::loadGroups(ServerBlock &block, const sol::table &table, Item *item) const {
	sol::object groupsObject = table["groups"];
	if (groupsObject.valid()) {
		if (groupsObject.get_type() == sol::type::table) {
			sol::table groupsTable = groupsObject.as<sol::table>();
			for (auto &groupObject : groupsTable) {
				std::string groupName = "group:" + groupObject.first.as<std::string>();
				u16 groupValue = groupObject.second.as<u16>();

				block.addGroup(groupName, groupValue);
				if (item)
					item->addGroup(groupName, groupValue);
			}
		}
		else
			gkError() << "For block" << block.stringID() << ": 'groups' must be a table";
	}
}

inline void LuaBlockLoader::loadParams(ServerBlock &block) const {
	if (block.isRotatable())
		block.param().allocateBits(BlockParam::Type::Rotation, 5);

	for (auto &it : block.states()) {
		if (it.drawType() == BlockDrawType::SubBoxes && it.subBoxesType() == BlockState::WallMounted)
			block.param().allocateBits(BlockParam::Type::WallMounted, 2);
	}

	if (block.states().size() > 1) {
		int bits = 1;
		int index = block.states().size();
		while (index >>= 1)
			++bits;
		block.param().allocateBits(BlockParam::Type::State, bits);
	}

	if (block.customParamBits()) {
		block.param().allocateBits(BlockParam::Type::Custom, block.customParamBits());
	}
}

