#version 120

varying vec4 v_coord3d;
varying vec4 v_color;
varying vec4 v_normal;
varying vec2 v_texCoord;
varying vec2 v_lightValue;
varying float v_ambientOcclusion;

varying float v_blockFace;
varying float v_dist;

uniform int u_renderDistance;

uniform sampler2D u_tex;

uniform vec4 u_skyColor;
uniform float u_sunlightIntensity;

// Get light color
vec4 light(vec4 color, vec3 lightColor, vec4 lightPosition, float ambientIntensity, float diffuseIntensity);

// Apply fog
vec4 fog(vec4 color, float fogCoord, float fogStart, float fogEnd);

void main() {
	// Needed to prevent bad interpolation on some systems
	// Refer to #23 for more informations
	float blockFace = floor(v_blockFace + 0.5);
	float lightCheck = floor(v_lightValue.x + 0.5);
	vec4 normal = floor(v_normal + 0.5);

	// Discard if the pixel is too far away
	if(blockFace > -1. && v_dist > u_renderDistance) discard;

	// Get current pixel color and apply multiplier on grayscale textures
	vec4 color = v_color;
	if (v_texCoord.x > -0.99 && v_texCoord.y > -0.99) {
		color = texture2D(u_tex, v_texCoord);
		if (blockFace > -1 && color.r == color.g && color.g == color.b) {
			color *= v_color;
		}
	}

	// Block breaking animation
	/* if (blockFace == -2 && color.a != 0.) { */
	/* 	if (color.r < 0.5) { */
	/* 		color.rgb = 0.5 * color.rgb; */
	/* 		color.a -= 0.2; */
	/* 	} */
	/* 	else { */
	/* 		color.rgb = 0.8 * color.rgb; */
	/* 		color.a -= 0.7; */
	/* 	} */
	/* } */

	// Very cheap "transparency": don't draw pixels with a low alpha value
	if(color.a < 0.3 && blockFace > -1.) discard;

	float minBrightness = 2.0 / 16.0;
	if (lightCheck != -1.) {
		float sunlight = clamp(v_lightValue.x * u_sunlightIntensity, 3, 15);

		float ambientIntensity = max(max(v_lightValue.x, v_lightValue.y) / 16.0, minBrightness);
		float diffuseIntensity = max(v_lightValue.x, v_lightValue.y) / 32.0;

		// These numbers should be in sync with enum BlockFace in TilesDef.hpp
		// Bottom
		if (normal.z == -1)
			ambientIntensity = max(ambientIntensity * 0.6, minBrightness);
		// West or East
		if (normal.x == 1 || normal.x == -1)
			ambientIntensity = max(ambientIntensity * 0.75, minBrightness);
		// South or North
		if (normal.y == 1 || normal.y == -1)
			ambientIntensity = max(ambientIntensity * 0.9, minBrightness);

		float lightval = clamp(sunlight / 15.0, v_lightValue.y / 15.0, 1.0);

		color = light(color, vec3(lightval, lightval, lightval), v_coord3d, ambientIntensity, diffuseIntensity);
	}

	color.rgb *= v_ambientOcclusion;

	color = fog(color, v_dist, u_renderDistance - 32, u_renderDistance);

	gl_FragColor = color;
}

