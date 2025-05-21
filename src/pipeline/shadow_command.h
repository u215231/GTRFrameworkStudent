#pragma once

#include "scene.h"

namespace GFX {
	class FBO;
	class Shader;
}

class ShadowCommand
{
public:
	int num_shadows = 0;											// Number of shadows
	int slots[MAX_NUM_LIGHTS] = { 0 };								// Slots of shadow maps
	float biases[MAX_NUM_LIGHTS] = { 0 };							// Shadow biases	
	Matrix44 view_projections[MAX_NUM_LIGHTS];						// View projections from the point of view of lights
	GFX::Texture* depth_textures[MAX_NUM_LIGHTS] = { nullptr };		// Shadow map textures 

	ShadowCommand();
	~ShadowCommand();

	void parseShadows(std::vector<Camera*> camera_light_list, std::vector<GFX::FBO*> shadow_FBOs);
	void uploadUniforms(GFX::Shader* shader) const;
	void uploadUniform(GFX::Shader* shader, int s) const;
};