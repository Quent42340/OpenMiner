#version 120

attribute vec4 color;
attribute vec4 coord3d;
attribute vec3 normal;
attribute vec2 texCoord;
attribute vec2 lightValue;
attribute float ambientOcclusion;

varying vec4 v_coord3d;
varying vec4 v_color;
varying vec4 v_normal;
varying vec2 v_texCoord;
varying vec2 v_lightValue;
varying float v_ambientOcclusion;

varying float v_blockFace;
varying float v_dist;

uniform mat4 u_modelMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_viewMatrix;

// uniform int u_time;

void main() {
	// FIXME: This code won't work anymore because of 'blockType' attibute removal
	//        One way to get those effects back would be to add another attribute 'effect'
	//        with values: 'NONE', 'LIQUID', 'LEAVES'
	// vec3 finalPos = coord3d.xyz;
	// if (blockType == 8) { // Water
	// 	// finalPos.z += sin((u_time / 1000.0 + mod(finalPos.x, 16)) * 1.75) / 10.0;
	// 	// finalPos.z += cos((u_time / 1000.0 + mod(finalPos.y, 16)) * 1.75) / 10.0;
	// 	// finalPos.z -= 0.25;
	// 	// finalPos.z -= 1.0 / 16.0;
	// 	finalPos.z += sin(u_time / 1000.0) / 16.0 - 0.125;
	// }
	// else if (blockType == 4) { // Leaves
	// 	finalPos.xy += sin((u_time / 1000.0 + finalPos.x) * 2) / 30.0;
	// 	finalPos.xy += cos((u_time / 1000.0 + finalPos.y) * 2) / 30.0;
	// }

	// Used for lighting
	v_coord3d = u_modelMatrix * vec4(coord3d.xyz, 1.0);
	v_normal = vec4(normal, 1.0);

	v_color = color;
	v_texCoord = texCoord;
	v_lightValue = lightValue;

	if (ambientOcclusion != 4) {
		const float aovalues[] = float[](0.2, 0.45, 0.75, 1.0);
		v_ambientOcclusion = aovalues[int(ambientOcclusion)];
	} else {
		v_ambientOcclusion = 1.0;
	}

	v_blockFace = coord3d.w;

	// Distance from eye
	v_dist = length(u_viewMatrix * v_coord3d);

	gl_Position = u_projectionMatrix * u_viewMatrix * v_coord3d;
}

