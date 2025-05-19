#pragma once
#include "scene.h"
#include "light.h"

namespace SCN {
	class LightEntity;
	class Shader;
}

class LightCommand
{
public:
	int num_lights = 0;								// Number of lights
	vec3 ambient;									// Ambient light (constant)
	vec3 positions[MAX_NUM_LIGHTS];					// Light position
	vec3 colors[MAX_NUM_LIGHTS];					// Light color
	vec3 directions[MAX_NUM_LIGHTS];				// Spotlight direction (D)
	vec2 cos_angles[MAX_NUM_LIGHTS];				// cos(alpha_min), cos(alpha_max)
	float intensities[MAX_NUM_LIGHTS] = { 0.0f };	// Light intensity
	int types[MAX_NUM_LIGHTS] = { 0 };				// Light type 

	LightCommand();
	~LightCommand();

	void parseLights(std::vector<SCN::LightEntity*> light_list, SCN::Scene* scene);
	void uploadUniforms(GFX::Shader* shader) const;
};