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

#include <sol/sol.hpp>

#include "Block.hpp"
#include "BlockParam.hpp"

void BlockParam::serialize(sf::Packet &packet) const {
	packet << m_totalSize << m_allocatedBits;
}

void BlockParam::deserialize(sf::Packet &packet) {
	packet >> m_totalSize >> m_allocatedBits;
}

void BlockParam::allocateBits(Type type, u8 size) {
	auto it = m_allocatedBits.find(type);
	if (it != m_allocatedBits.end()) {
		gkError() << "Can't allocate param type" << getTypeName(type) << "twice in block" << m_block->stringID();
	}
	else if (m_totalSize + size <= 16) {
		m_allocatedBits.emplace(type, Param{m_totalSize, size});

		m_totalSize += size;

		// gkDebug() << "Allocated" << (int)size << "bits for type" << getTypeName(type) << "in block" << m_block.stringID();
	}
	else {
		gkError() << "Failed to allocate bits for param" << getTypeName(type) << "in block" << m_block->stringID();
		gkError() << "Reason: Can't allocate more than 16 bits. Allocated bits:";
		for (auto &it : m_allocatedBits) {
			gkError() << "\t-" << getTypeName((Type)it.first) << "=" << (int)it.second.size;
		}
	}
}

u16 BlockParam::getParam(Type type, u16 data) const {
	auto it = m_allocatedBits.find(type);
	if (it == m_allocatedBits.end()) {
		gkError() << "Failed to get param" << getTypeName(type) << "in block" << m_block->stringID();
		gkError() << "Reason: This param type has not been allocated.";
		return 0;
	}

	return (data >> it->second.offset) & ~(~0u << it->second.size);
}

u16 BlockParam::setParam(Type type, u16 data, u16 param) const {
	auto it = m_allocatedBits.find(type);
	if (it == m_allocatedBits.end()) {
		gkError() << "Failed to set param" << getTypeName(type) << "in block" << m_block->stringID();
		gkError() << "Reason: This param type has not been allocated.";
		return 0;
	}

	u16 mask = ~(~0u << it->second.size) << it->second.offset;
	param <<= it->second.offset;
	if ((param & ~mask) != 0)
		gkWarning() << "Block param overflow for type" << getTypeName(type) << "in block" << m_block->stringID();

	return (data & ~mask) | (param & mask);
}

std::string BlockParam::getTypeName(Type type) {
	static std::array<std::string, Type::Count> names = {
		"Rotation",
		"State",
		"WallMounted",
		"Custom",
	};

	return names[type];
}

// Please update 'docs/lua-api-cpp.md' if you change this
void BlockParam::initUsertype(sol::state &lua) {
	lua.new_usertype<BlockParam>("BlockParam",
		"get_param", &BlockParam::getParam,
		"set_param", &BlockParam::setParam,
		"has_param", &BlockParam::hasParam
	);

	lua["BlockParamType"] = lua.create_table_with(
		"Rotation",    Type::Rotation,
		"State",       Type::State,
		"WallMounted", Type::WallMounted,
		"Custom",      Type::Custom,

		"Count", Type::Count
	);
}

