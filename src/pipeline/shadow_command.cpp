#include "shadow_command.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"


ShadowCommand::ShadowCommand()
{
}

ShadowCommand::~ShadowCommand()
{
}

void ShadowCommand::parseShadows(std::vector<Camera*> camera_light_list, std::vector<GFX::FBO*> shadow_FBOs) {
	this->num_shadows = min(MAX_NUM_LIGHTS, (int)camera_light_list.size());
	int s = 0;
	for (Camera* light_camera : camera_light_list) {
		this->slots[s] = 2 + s;
		this->depth_textures[s] = shadow_FBOs.at(s)->depth_texture;
		this->view_projections[s] = light_camera->viewprojection_matrix;
		if (this->biases[s] == 0.0f) {
			this->biases[s] = 0.005f;
		}
		s++;
	}
}

void ShadowCommand::uploadUniforms(GFX::Shader* shader) const
{
	const int m = this->num_shadows;
	for (int j = 0; j < m; j++) {
		glActiveTexture(GL_TEXTURE0 + this->slots[j]);
		glBindTexture(GL_TEXTURE_2D, this->depth_textures[j]->texture_id);
	}

	shader->setUniform("u_num_shadows", m);
	shader->setUniform1Array("u_shadow_maps", (int*)this->slots, m);
	shader->setMatrix44Array("u_shadow_vps", (Matrix44*)this->view_projections, m);
	shader->setUniform1Array("u_shadow_biases", (float*)this->biases, m);
}

void ShadowCommand::uploadUniform(GFX::Shader* shader, int shadow_num) const
{
	if (shadow_num > this->num_shadows - 1)
		return;

	const int s = shadow_num;
	shader->setUniform("u_shadow_num", shadow_num);
	shader->setTexture("u_shadow_map", this->depth_textures[s], s + 2);
	shader->setUniform("u_shadow_vp", this->view_projections[s]);
	shader->setUniform("u_shadow_bias", this->biases[s]);
}