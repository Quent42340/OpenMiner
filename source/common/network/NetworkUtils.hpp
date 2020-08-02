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
#ifndef NETWORKUTILS_HPP_
#define NETWORKUTILS_HPP_

#include <SFML/Network/Packet.hpp>

//======================================================================================
// gk::Rect
//======================================================================================
#include <gk/core/Rect.hpp>

template<typename T>
sf::Packet &operator<<(sf::Packet &packet, const gk::Rect<T> &rect) {
	packet << rect.x << rect.y << rect.sizeX << rect.sizeY;
	return packet;
}

template<typename T>
sf::Packet &operator>>(sf::Packet &packet, gk::Rect<T> &rect) {
	packet >> rect.x >> rect.y >> rect.sizeX >> rect.sizeY;
	return packet;
}

//======================================================================================
// gk::Box
//======================================================================================
#include <gk/core/Box.hpp>

template<typename T>
sf::Packet &operator<<(sf::Packet &packet, const gk::Box<T> &box) {
	packet << box.x << box.y << box.z << box.sizeX << box.sizeY << box.sizeZ;
	return packet;
}

template<typename T>
sf::Packet &operator>>(sf::Packet &packet, gk::Box<T> &box) {
	packet >> box.x >> box.y >> box.z >> box.sizeX >> box.sizeY >> box.sizeZ;
	return packet;
}

//======================================================================================
// gk::Vector3
//======================================================================================
#include <gk/core/Vector3.hpp>

template<typename T>
sf::Packet &operator<<(sf::Packet &packet, const gk::Vector3<T> &vec) {
	packet << vec.x << vec.y << vec.z;
	return packet;
}

template<typename T>
sf::Packet &operator>>(sf::Packet &packet, gk::Vector3<T> &vec) {
	packet >> vec.x >> vec.y >> vec.z;
	return packet;
}

//======================================================================================
// gk::Color
//======================================================================================
#include <gk/graphics/Color.hpp>

sf::Packet &operator<<(sf::Packet &packet, const gk::Color &color);
sf::Packet &operator>>(sf::Packet &packet, gk::Color &color);

//======================================================================================
// entt::entity
//======================================================================================
#include <entt/entt.hpp>

sf::Packet &operator<<(sf::Packet &packet, const entt::entity &entity);
sf::Packet &operator>>(sf::Packet &packet, entt::entity &entity);

//======================================================================================
// std::vector
//======================================================================================
#include <vector>

template<typename T>
sf::Packet &operator<<(sf::Packet &packet, const std::vector<T> &vec) {
	packet << (unsigned int)vec.size();
	for (const T &it : vec)
		packet << it;
	return packet;
}

template<typename T>
sf::Packet &operator>>(sf::Packet &packet, std::vector<T> &vec) {
	unsigned int size;
	packet >> size;

	for (unsigned int i = 0 ; i < size ; ++i) {
		T v;
		packet >> v;
		vec.emplace_back(v);
	}

	return packet;
}

//======================================================================================
// std::deque
//======================================================================================
#include <deque>

template<typename T>
sf::Packet &operator<<(sf::Packet &packet, const std::deque<T> &deq) {
	packet << (unsigned int)deq.size();
	for (auto &it : deq)
		packet << it;
	return packet;
}

template<typename T>
sf::Packet &operator>>(sf::Packet &packet, std::deque<T> &deq) {
	unsigned int size;
	packet >> size;

	for (unsigned int i = 0 ; i < size ; ++i) {
		T v;
		packet >> v;
		deq.emplace_back(v);
	}

	return packet;
}

//======================================================================================
// std::unordered_map
//======================================================================================
#include <unordered_map>

template<typename T, typename U>
sf::Packet &operator<<(sf::Packet &packet, const std::unordered_map<T, U> &map) {
	packet << (unsigned int)map.size();
	for (auto &it : map)
		packet << it.first << it.second;
	return packet;
}

template<typename T, typename U>
sf::Packet &operator>>(sf::Packet &packet, std::unordered_map<T, U> &map) {
	unsigned int size;
	packet >> size;

	for (unsigned int i = 0 ; i < size ; ++i) {
		T k;
		U v;
		packet >> k >> v;
		map.emplace(k, v);
	}

	return packet;
}

//======================================================================================
// std::array
//======================================================================================
#include <array>

template<typename T, std::size_t N>
sf::Packet &operator<<(sf::Packet &packet, const std::array<T, N> &array) {
	packet << (unsigned int)N;
	for (const T &it : array)
		packet << it;
	return packet;
}

template<typename T, std::size_t N>
sf::Packet &operator>>(sf::Packet &packet, std::array<T, N> &array) {
	unsigned int size;
	packet >> size;

	for (unsigned int i = 0 ; i < size ; ++i) {
		packet >> array[i];
	}

	return packet;
}

#endif // NETWORKUTILS_HPP_
