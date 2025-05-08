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
	int j = 0;
	for (Camera* light_camera : camera_light_list) {
		this->slots[j] = 2 + j;
		this->depth_textures[j] = shadow_FBOs.at(j)->depth_texture;
		this->view_projections[j] = light_camera->viewprojection_matrix;
		if (this->biases[j] == 0.0f) {
			this->biases[j] = 0.005f;
		}
		j++;
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