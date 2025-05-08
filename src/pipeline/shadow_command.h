#pragma once

#include "scene.h"

namespace GFX {
	class FBO;
	class Shader;
}

class ShadowCommand
{
public:
	int num_shadows = 0;
	int slots[MAX_NUM_LIGHTS] = { 0 };
	float biases[MAX_NUM_LIGHTS] = { 0 };
	Matrix44 view_projections[MAX_NUM_LIGHTS];
	GFX::Texture* depth_textures[MAX_NUM_LIGHTS] = { nullptr };

	ShadowCommand();
	~ShadowCommand();

	void parseShadows(std::vector<Camera*> camera_light_list, std::vector<GFX::FBO*> shadow_FBOs);
	void uploadUniforms(GFX::Shader* shader) const;
};