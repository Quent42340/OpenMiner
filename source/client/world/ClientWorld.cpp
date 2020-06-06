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
#include <glm/gtx/norm.hpp>

#include <gk/gl/GLCheck.hpp>
#include <gk/gl/Shader.hpp>
#include <gk/resource/ResourceHandler.hpp>

#include "ClientCommandHandler.hpp"
#include "ClientPlayer.hpp"
#include "ClientWorld.hpp"
#include "Registry.hpp"
#include "Sky.hpp"
#include "TextureAtlas.hpp"
#include "World.hpp"

ClientWorld::ClientWorld() : m_textureAtlas(gk::ResourceHandler::getInstance().get<TextureAtlas>("atlas-blocks"))
{
}

void ClientWorld::update() {
	// Update loaded chunks
	for (auto it = m_chunks.begin() ; it != m_chunks.end() ;) {
		// If chunk is too far, remove it
		if (it->second->isTooFar() && (it->second->isInitialized() || it->second->areAllNeighboursTooFar())) {
			removeChunk(it);
		}
		// Otherwise, update the chunk
		else {
			if (World::isReloadRequested)
				it->second->setChanged(true);

			// if (it->second->areAllNeighboursInitialized())
			it->second->update();

			++it;
		}
	}

	World::isReloadRequested = false;

	sendChunkRequests();

	m_scene.update();
}

void ClientWorld::sendChunkRequests() {
	s32 ux = m_closestInitializedChunk.x;
	s32 uy = m_closestInitializedChunk.y;
	s32 uz = m_closestInitializedChunk.z;
	float ud = m_closestInitializedChunk.w;

	// If we have a chunk marked for initialization
	if (ud < 1000000.0) {
		ClientChunk *chunk = (ClientChunk *)getChunk(ux, uy, uz);
		if(chunk && !chunk->hasBeenRequested()) {
			// Send a chunk request to the server
			m_client->sendChunkRequest(ux, uy, uz);
			chunk->setHasBeenRequested(true);

			// std::cout << "Chunk at (" << ux << ", " << uy << ", " << uz << ") requested" << std::endl;
		}
	}
}

void ClientWorld::checkPlayerChunk(double playerX, double playerY, double playerZ) {
	int pcx = std::floor(playerX / CHUNK_WIDTH);
	int pcy = std::floor(playerY / CHUNK_DEPTH);
	int pcz = std::floor(playerZ / CHUNK_HEIGHT);

	ClientChunk *chunk = (ClientChunk *)getChunk(pcx, pcy, pcz);
	if (!chunk) {
		m_chunks.emplace(gk::Vector3i{pcx, pcy, pcz}, new ClientChunk(pcx, pcy, pcz, *m_dimension, *this, m_textureAtlas));
	}
}

void ClientWorld::clear() {
	m_chunks.clear();
	m_scene.registry().clear();
}

void ClientWorld::changeDimension(u16 dimensionID) {
	const Dimension &dimension = Registry::getInstance().getDimension(dimensionID);
	m_dimension = &dimension;

	const Sky &sky = Registry::getInstance().getSkyFromStringID(dimension.sky());
	m_sky = &sky;
}

void ClientWorld::receiveChunkData(Network::Packet &packet) {
	s32 cx, cy, cz;
	packet >> cx >> cy >> cz;

	// Get the chunk from the map or create it if it doesn't exist
	ClientChunk *chunk = (ClientChunk *)getChunk(cx, cy, cz);
	if (!chunk) {
		auto it = m_chunks.emplace(gk::Vector3i{cx, cy, cz}, new ClientChunk(cx, cy, cz, *m_dimension, *this, m_textureAtlas));
		chunk = it.first->second.get();
	}

	createChunkNeighbours(chunk);

	// Receive chunk data
	for (u16 z = 0 ; z < CHUNK_HEIGHT ; ++z) {
		for (u16 y = 0 ; y < CHUNK_DEPTH ; ++y) {
			for (u16 x = 0 ; x < CHUNK_WIDTH ; ++x) {
				u32 block;
				u8 light;

				packet >> block >> light;

				chunk->setBlockRaw(x, y, z, block & 0xffff);
				chunk->setData(x, y, z, block >> 16);
				chunk->lightmap().setLightData(x, y, z, light);
			}
		}
	}

	if (!chunk->isInitialized()) {
		chunk->getSurroundingChunk(0)->setChanged(true);
		chunk->getSurroundingChunk(1)->setChanged(true);
		chunk->getSurroundingChunk(2)->setChanged(true);
		chunk->getSurroundingChunk(3)->setChanged(true);
		chunk->getSurroundingChunk(4)->setChanged(true);
		chunk->getSurroundingChunk(5)->setChanged(true);
	}

	chunk->setInitialized(true);

	// if (cx == 2 && cy == 0 && cz == 1)
	// 	std::cout << "Chunk at (" << cx << ", " << cy << ", " << cz << ") received" << std::endl;
}

void ClientWorld::removeChunk(ChunkMap::iterator &it) {
	ClientChunk *surroundingChunks[6] = {
		(ClientChunk *)it->second->getSurroundingChunk(0),
		(ClientChunk *)it->second->getSurroundingChunk(1),
		(ClientChunk *)it->second->getSurroundingChunk(2),
		(ClientChunk *)it->second->getSurroundingChunk(3),
		(ClientChunk *)it->second->getSurroundingChunk(4),
		(ClientChunk *)it->second->getSurroundingChunk(5)
	};

	it = m_chunks.erase(it);

	for (u8 i = 0 ; i < 6 ; ++i) {
		if (surroundingChunks[i]) {
			surroundingChunks[i]->setSurroundingChunk((i % 2 == 0) ? i + 1 : i - 1, nullptr);

			if (!surroundingChunks[i]->isTooFar())
				createChunkNeighbours(surroundingChunks[i]);
		}
	}
}

Chunk *ClientWorld::getChunk(int cx, int cy, int cz) const {
	auto it = m_chunks.find({cx, cy, cz});
	if (it == m_chunks.end())
		return nullptr;

	return it->second.get();
}

void ClientWorld::createChunkNeighbours(ClientChunk *chunk) {
	gk::Vector3i surroundingChunks[6] = {
		{chunk->x() - 1, chunk->y(),     chunk->z()},
		{chunk->x() + 1, chunk->y(),     chunk->z()},
		{chunk->x(),     chunk->y() - 1, chunk->z()},
		{chunk->x(),     chunk->y() + 1, chunk->z()},
		{chunk->x(),     chunk->y(),     chunk->z() - 1},
		{chunk->x(),     chunk->y(),     chunk->z() + 1},
	};

	// Create entries in the map for surrounding chunks
	// That will allow draw() to mark them for initialization
	for (u8 i = 0 ; i < 6 ; ++i) {
		const s32 scx = surroundingChunks[i].x;
		const s32 scy = surroundingChunks[i].y;
		const s32 scz = surroundingChunks[i].z;

		ClientChunk *neighbour = (ClientChunk *)getChunk(scx, scy, scz);
		if (!neighbour) {
			auto it = m_chunks.emplace(gk::Vector3i{scx, scy, scz}, new ClientChunk(scx, scy, scz, *m_dimension, *this, m_textureAtlas));
			neighbour = it.first->second.get();
		}

		chunk->setSurroundingChunk(i, neighbour);
		neighbour->setSurroundingChunk((i % 2 == 0) ? i + 1 : i - 1, chunk);
	}
}

void ClientWorld::draw(gk::RenderTarget &target, gk::RenderStates states) const {
	// glClearColor used to be called in changeDimension(), but when "toggling fullscreen mode"
	// with SFML, you actually recreate a window. Thus, all the glEnable/glDisable/glClearColor
	// states are cleared when fullscreen mode is enabled/disabled.
	if (m_sky)
		glCheck(glClearColor(m_sky->color().r, m_sky->color().g, m_sky->color().b, m_sky->color().a));

	if (!target.getView() || !m_camera) {
		gkError() << "Trying to draw world without a camera";
		return;
	}

	states.vertexAttributes = VertexAttribute::All;

	gk::Shader::bind(states.shader);

	states.shader->setUniform("u_renderDistance", Config::renderDistance * CHUNK_WIDTH);

	if (m_sky)
		states.shader->setUniform("u_fogColor", m_sky->fogColor());

	gk::Shader::bind(nullptr);

	m_closestInitializedChunk = gk::Vector4f{0, 0, 0, 1000000};

	// Changing the values sent to the GPU to double precision is suicidal,
	// performance wise, if possible at all. Therefore we want to keep the
	// GL rendering numbers in single precision format. But that introduces
	// an issue at larger coordinates, because the precision of floats
	// quickly degrades as the numbers grow, with a random wobbling being
	// very noticeable at e.g. coordinates >= 65536 or so, and the waving
	// leaves effect being very jerky in comparison with the effect near the
	// origin.
	//
	// To gain rendering precision, we subtract the camera position from the
	// coordinates of the models to be rendered, to make them all small in
	// relation to the camera, prior to converting them to floats. Then the
	// camera itself is moved to (0, 0, 0) for rendering purposes. Now the
	// vertex coordinates passed to the renderer are all small, and single
	// precision floats suffice for the drawing.

	gk::Vector3d cameraPos(m_camera->getDPosition());
	m_camera->setDPosition(0, 0, 0);  // Temporarily move the camera to the origin

	std::vector<std::pair<ClientChunk*, gk::Transform>> chunks;
	for(auto &it : m_chunks) {
		gk::Transform tf = glm::translate(glm::mat4(1.0f),
		                                  glm::vec3(it.second->x() * CHUNK_WIDTH  - cameraPos.x,
		                                            it.second->y() * CHUNK_DEPTH  - cameraPos.y,
		                                            it.second->z() * CHUNK_HEIGHT - cameraPos.z));

		// Is the chunk close enough?
		glm::vec4 center = target.getView()->getViewTransform().getMatrix()
		                 * tf.getMatrix()
		                 * glm::vec4(CHUNK_WIDTH / 2, CHUNK_DEPTH / 2, CHUNK_HEIGHT / 2, 1);

		// Nope, too far, don't render it
		if(glm::length(center) > (Config::renderDistance + 1) * CHUNK_WIDTH) {
			it.second->setTooFar(true);
			continue;
		}

		it.second->setTooFar(false);

		// Is this chunk's centre on the screen?
		// float d = glm::length2(center);
		center.x /= center.w;
		center.y /= center.w;

		// If it is behind the camera, don't bother drawing it.
		// Our screen coordinates are +X right, +Y up, and for a right-handed
		// coordinate system, depth must be negative Z, so anything with a
		// positive Z is behind the camera.
		if (center.z > CHUNK_MAXSIZE / 2) {
			continue;
		}

		// If it is outside the screen, don't bother drawing it
		if (fabsf(center.x) > 1 + fabsf(CHUNK_HEIGHT * 2 / center.w)
		 || fabsf(center.y) > 1 + fabsf(CHUNK_HEIGHT * 2 / center.w)) {
			// FIXME: Disable this test; one that considers all eight corners of the chunk is needed instead.
			//continue;
		}

		// If this chunk is not initialized, skip it
		if(!it.second->isInitialized() && !it.second->hasBeenRequested()) {
			// // But if it is the closest to the camera, mark it for initialization
			// if(d < m_closestInitializedChunk.w) {
			// 	m_closestInitializedChunk.w = d;
			// 	m_closestInitializedChunk.x = it.second->x();
			// 	m_closestInitializedChunk.y = it.second->y();
			// 	m_closestInitializedChunk.z = it.second->z();
			// }

			continue;
		}

		chunks.emplace_back(it.second.get(), tf);
	}

	for (u8 i = 0 ; i < ChunkBuilder::layers ; ++i) {
		for (auto &it : chunks) {
			states.transform = it.second;
			it.first->drawLayer(target, states, i);
		}
	}

	m_camera->setDPosition(cameraPos);  // Restore the camera to its original position

	states.transform = gk::Transform::Identity;
	target.draw(m_scene, states);
}

