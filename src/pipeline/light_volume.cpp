#include "light_volume.h"

#include <algorithm> //sort

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/prefab.h"
#include "../pipeline/material.h"
#include "../pipeline/animation.h"
#include "../utils/utils.h"
#include "../extra/hdre.h"
#include "../core/ui.h"

#include "scene.h"

LightVolume::LightVolume()
{
}

LightVolume::~LightVolume()
{
}

/*
void Renderer::parseLightVolumes(std::vector<LightEntity*> light_list)
{
	for (GFX::Mesh sphere : this->spheres) {
		sphere.clear();
	}
	this->spheres.clear();
	for (SCN::LightEntity* light : light_list) {
		if (light->light_type == eLightType::POINT
			or light->light_type == eLightType::SPOT) {

			GFX::Mesh sphere;
			sphere.createSphere(light->max_distance);
			this->spheres.push_back(sphere);
		}
	}
}
*/

/*
// For light volumes: still in testing
std::vector<SCN::LightEntity*> directional_light_list;
for (SCN::LightEntity* light : this->light_list) {
	if (light->light_type == SCN::eLightType::DIRECTIONAL) {
		directional_light_list.push_back(light);
	}
}
*/

//does not work
void LightVolume::render()
{
/*
	glDisable(GL_BLEND);

	glClearColor(
		scene->background_color.x,
		scene->background_color.y,
		scene->background_color.z,
		1.0
	);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	if (this->skybox_cubemap) {
		this->renderSkybox(this->skybox_cubemap);
	}

	for (DrawCommand draw_command : this->draw_command_opaque_list) {
		this->renderShaderSinglePass(Camera::current, draw_command, "forward_light_single_pass");
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (DrawCommand draw_command : this->draw_command_transparent_list) {
		this->renderShaderSinglePass(Camera::current, draw_command, "forward_light_single_pass");
	}

	glDisable(GL_BLEND);


	this->deferred_command.gbuffer_FBO->depth_texture->copyTo(lighting.gbuffer_FBO->depth_texture);

	this->lighting.bind();

	// Set the OpenGL config
	glDepthFunc(GL_GREATER);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	glFrontFace(GL_CW);

	for (SCN::LightEntity* light : this->light_list)
	{
		if (light->light_type == eLightType::DIRECTIONAL
			or light->light_type == eLightType::NO_LIGHT)
			continue;

		Vector3 light_position = light->root.getGlobalMatrix().getTranslation();

		Matrix44 model;
		model.setTranslation(light_position.x, light_position.y, light_position.z);
		model.scale(light->max_distance, light->max_distance, light->max_distance);

		GFX::Mesh sphere;
		sphere.createSphere(light->max_distance);

		GFX::Shader* shader = GFX::Shader::Get("light_volumes");
		if (!shader)
			return;
		shader->enable();

		shader->setUniform("u_model", model);
		shader->setUniform("u_light_position", light_position);
		shader->setUniform("u_light_color", light->color);
		shader->setUniform("u_light_max_distance", light->max_distance);

		sphere.render(GL_TRIANGLES);
	}

	// Return the OpenGL config to what it was
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);
	glFrontFace(GL_CCW);

	lighting.unbind();

	lighting.gbuffer_FBO->color_textures[0]->toViewport();
*/
}