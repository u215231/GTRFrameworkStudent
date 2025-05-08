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
	int num_lights = 0;
	vec3 ambient;
	vec3 positions[MAX_NUM_LIGHTS];
	vec3 colors[MAX_NUM_LIGHTS];
	vec3 directions[MAX_NUM_LIGHTS];
	float intensities[MAX_NUM_LIGHTS] = { 0.0f };
	float cos_angle_max[MAX_NUM_LIGHTS] = { 0.0f };
	float cos_angle_min[MAX_NUM_LIGHTS] = { 0.0f };
	int types[MAX_NUM_LIGHTS] = { 0 };

	LightCommand();
	~LightCommand();

	void parseLights(std::vector<SCN::LightEntity*> light_list, SCN::Scene* scene);
	void uploadUniforms(GFX::Shader* shader) const;
};