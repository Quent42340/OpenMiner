--
-- =====================================================================================
--
--  OpenMiner
--
--  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
--  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
--
--  This file is part of OpenMiner.
--
--  OpenMiner is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public
--  License as published by the Free Software Foundation; either
--  version 2.1 of the License, or (at your option) any later version.
--
--  OpenMiner is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public License
--  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
--  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
--
-- =====================================================================================
--

function open_door(pos, world)
	local data = world:get_block_data(pos.x, pos.y, pos.z)
	if not data then return end

	-- FIXME: Add a helper to get rotation from data
	local rotation = world:get_data(pos.x, pos.y, pos.z)

	local is_opened = data.meta:get_bool("is_opened") or false
	if not is_opened then
		world:set_data(pos.x, pos.y, pos.z, (rotation - 1) % 4)
	else
		world:set_data(pos.x, pos.y, pos.z, (rotation + 1) % 4)
	end

	data.meta:set_bool("is_opened", not is_opened)
end

mod:block {
	id = "door_wood_upper",
	name = "Wooden Door (WIP)",
	tiles = "door_wood_upper.png",
	inventory_image = "door_wood.png",
	groups = {
		ci_ignore = 1,
		om_material_wood = 1
	},

	is_rotatable = true,
	is_opaque = false,

	draw_type = "subboxes",

	subboxes = {
		type = "fixed",
		fixed = {13 / 16, 0, 0, 3 / 16, 1, 1},
	},

	on_block_placed = function(pos, world)
		world:add_block_data(pos.x, pos.y, pos.z, 0, 0)
	end,

	on_block_destroyed = function(pos, world)
		local lower_block_id = openminer.registry:get_block_from_string("default:door_wood_lower"):id()
		if world:get_block(pos.x, pos.y, pos.z - 1) == lower_block_id then
			world:set_block(pos.x, pos.y, pos.z - 1, 0)
		end
	end,

	on_block_activated = function(pos, block, player, world, client, server)
		open_door(pos, world)
		open_door({x = pos.x, y = pos.y, z = pos.z - 1}, world)
	end
}

mod:block {
	id = "door_wood_lower",
	name = "Wooden Door (WIP)",
	tiles = "door_wood_lower.png",
	inventory_image = "door_wood.png",
	groups = {
		om_material_wood = 1
	},

	is_rotatable = true,
	is_opaque = false,

	draw_type = "subboxes",

	subboxes = {
		type = "fixed",
		fixed = {13 / 16, 0, 0, 3 / 16, 1, 1},
	},

	on_block_placed = function(pos, world)
		world:add_block_data(pos.x, pos.y, pos.z, 0, 0)

		if world:get_block(pos.x, pos.y, pos.z + 1) == 0 then
			local data = world:get_data(pos.x, pos.y, pos.z)
			world:set_data(pos.x, pos.y, pos.z + 1, data)

			local upper_block_id = openminer.registry:get_block_from_string("default:door_wood_upper"):id()
			world:set_block(pos.x, pos.y, pos.z + 1, upper_block_id)
		end
	end,

	on_block_destroyed = function(pos, world)
		local upper_block_id = openminer.registry:get_block_from_string("default:door_wood_upper"):id()
		if world:get_block(pos.x, pos.y, pos.z + 1) == upper_block_id then
			world:set_block(pos.x, pos.y, pos.z + 1, 0)
		end
	end,

	on_block_activated = function(pos, block, player, world, client, server)
		open_door(pos, world)
		open_door({x = pos.x, y = pos.y, z = pos.z + 1}, world)
	end
}


