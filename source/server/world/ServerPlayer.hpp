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
#ifndef SERVERPLAYER_HPP_
#define SERVERPLAYER_HPP_

#include <unordered_set>

#include <gk/core/Debug.hpp>

#include "ClientInfo.hpp"
#include "Player.hpp"

class ServerPlayer : public Player {
	public:
		ServerPlayer(ClientInfo &client);

		const ClientInfo &client() const { return m_client; }

		const ItemStack &heldItemStack() { return m_inventory.getStack(m_heldItemSlot, 0); }
		void setHeldItemSlot(u8 heldItemSlot) { m_heldItemSlot = heldItemSlot; }

		bool isChunkLoaded(const gk::Vector3i &chunk) { return m_loadedChunks.find(chunk) != m_loadedChunks.end(); }
		void addLoadedChunk(const gk::Vector3i &chunk) { m_loadedChunks.emplace(chunk); }
		void removeLoadedChunk(const gk::Vector3i &chunk) { m_loadedChunks.erase(chunk); }

		static void initUsertype(sol::state &lua);

	private:
		std::unordered_set<gk::Vector3i> m_loadedChunks;

		ClientInfo &m_client;

		u8 m_heldItemSlot = 0;
};

#endif // SERVERPLAYER_HPP_
