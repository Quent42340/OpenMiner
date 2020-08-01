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
#ifndef VERTEX_HPP_
#define VERTEX_HPP_

#include <gk/gl/OpenGL.hpp>

struct Vertex {
	GLfloat coord3d[4]      = {0, 0, 0, 1};
	GLfloat texCoord[2]     = {-1, -1};
	GLfloat color[4]        = {0, 0, 0, 1};
	GLfloat normal[3]       = {0, 0, 0};
	GLfloat lightValue[2]   = {-1, -1};
	GLfloat ambientOcclusion = 4;
};

namespace VertexAttribute {
	enum {
		Coord3d          = 8,
		TexCoord         = 16,
		Color            = 32,
		Normal           = 64,
		LightValue       = 128,
		AmbientOcclusion = 256,

		Basic            = Coord3d | TexCoord | Color,
		All              = Basic | Normal | LightValue | AmbientOcclusion
	};
}

#endif // VERTEX_HPP_
