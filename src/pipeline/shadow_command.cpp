#include "shadow_command.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"
#include "../pipeline/light.h"
#include "../pipeline/renderer.h"

ShadowCommand::ShadowCommand()
{
	this->num_shadows = 0;
}

ShadowCommand::~ShadowCommand()
{
	this->resizeShadowFBOs(0);
}

void ShadowCommand::parseCameraLights(std::vector<SCN::LightEntity*> light_list)
{
	for (Camera* camera : this->camera_light_list) {
		delete camera;
	}
	this->camera_light_list.clear();
	for (SCN::LightEntity* light : light_list) {
		Camera* camera = light->getCamera();
		if (camera) {
			this->camera_light_list.push_back(camera);
		}
	}
}

void ShadowCommand::resizeShadowFBOs(int num_lights) 
{
	while (this->num_shadows < min(MAX_NUM_LIGHTS, num_lights)) {
		GFX::FBO* shadow_FBO = new GFX::FBO();
		shadow_FBO->setDepthOnly(1024, 1024);
		this->shadow_FBOs.push_back(shadow_FBO);
		this->num_shadows++;
	}
	while (max(0, num_lights) < this->num_shadows) {
		GFX::FBO* shadow_FBO = this->shadow_FBOs.at(this->num_shadows - 1);
		this->shadow_FBOs.pop_back();
		delete shadow_FBO;
		this->num_shadows--;
	}
}

void ShadowCommand::parseShadows(std::vector<SCN::LightEntity*> light_list) 
{
	this->parseCameraLights(light_list);

	int num_lights = (int)camera_light_list.size();
	this->resizeShadowFBOs(num_lights);

	for (int i = 0; i < num_lights; i++) {
		this->slots[i] = 2 + i;
		this->depth_textures[i] = shadow_FBOs.at(i)->depth_texture;
		this->view_projections[i] = camera_light_list.at(i)->viewprojection_matrix;
		this->biases[i] = this->biases[i] == 0.0f ? 0.005f : this->biases[i];
	}
}

void ShadowCommand::renderShadows(SCN::Renderer* renderer)
{
	for (int i = 0; i < (int)this->camera_light_list.size(); i++) {
		Camera* camera_light = this->camera_light_list.at(i);
		GFX::FBO* shadow_fbo = this->shadow_FBOs.at(i);
		renderer->renderShadow(camera_light, shadow_fbo);
	}
}

void ShadowCommand::uploadUniforms(GFX::Shader* shader) const
{
	const int n = this->num_shadows;
	for (int i = 0; i < n; i++) {
		glActiveTexture(GL_TEXTURE0 + this->slots[i]);
		glBindTexture(GL_TEXTURE_2D, this->depth_textures[i]->texture_id);
	}

	shader->setUniform("u_num_shadows", n);
	shader->setUniform1Array("u_shadow_maps", (int*)this->slots, n);
	shader->setMatrix44Array("u_shadow_vps", (Matrix44*)this->view_projections, n);
	shader->setUniform1Array("u_shadow_biases", (float*)this->biases, n);
}

void ShadowCommand::uploadUniform(GFX::Shader* shader, int shadow_num) const
{
	if (this->num_shadows - 1 < shadow_num)
		return;

	const int i = shadow_num;
	shader->setUniform("u_shadow_num", shadow_num);
	shader->setTexture("u_shadow_map", this->depth_textures[i], i + 2);
	shader->setUniform("u_shadow_vp", this->view_projections[i]);
	shader->setUniform("u_shadow_bias", this->biases[i]);
}