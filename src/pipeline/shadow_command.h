#pragma once

#include "scene.h"

namespace SCN {
	class LightEntity;
	class Renderer;
}

namespace GFX {
	class FBO;
	class Shader;
}

class ShadowCommand
{
public:
	int num_shadows;												
	int slots[MAX_NUM_LIGHTS] = { 0 };								
	float biases[MAX_NUM_LIGHTS] = { 0 };							
	Matrix44 view_projections[MAX_NUM_LIGHTS];						
	GFX::Texture* depth_textures[MAX_NUM_LIGHTS] = { nullptr };	
	std::vector<GFX::FBO*> shadow_FBOs;
	std::vector<Camera*> camera_light_list;

	ShadowCommand();
	~ShadowCommand();

	void resizeShadowFBOs(int num_lights);
	void parseCameraLights(std::vector<SCN::LightEntity*> light_list);
	void parseShadows(std::vector<SCN::LightEntity*> light_list);
	void renderShadows(SCN::Renderer* renderer);
	void uploadUniforms(GFX::Shader* shader) const;
	void uploadUniform(GFX::Shader* shader, int s) const;
	
};