#include "light_command.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"

LightCommand::LightCommand()
{
}

LightCommand::~LightCommand()
{
}

void LightCommand::parseLights(std::vector<SCN::LightEntity*> light_list, SCN::Scene* scene)
{
	this->num_lights = min(MAX_NUM_LIGHTS, (int)light_list.size());
	this->ambient = scene->ambient_light;
	int i = 0;
	for (SCN::LightEntity* light : light_list) {
		this->positions[i] = light->root.getGlobalMatrix().getTranslation();
		this->intensities[i] = light->intensity;
		this->types[i] = light->light_type;
		this->colors[i] = light->color;

		if (light->light_type == SCN::eLightType::DIRECTIONAL) {
			this->directions[i] = light->root.getGlobalMatrix().frontVector();
			this->cos_angles[i] = vec2(0.0f, 0.0f);
		}
		else if (light->light_type == SCN::eLightType::SPOT) {
			this->directions[i] = light->root.getGlobalMatrix().frontVector();
			this->cos_angles[i] = light->getConeInfo();
		}
		else {
			this->directions[i] = vec3(0.0f);
			this->cos_angles[i] = vec2(0.0f, 0.0f);
		}
		i++;
	}
}

void LightCommand::uploadUniforms(GFX::Shader* shader) const
{
	const int n = this->num_lights;
	shader->setUniform("u_num_lights", n);
	shader->setUniform("u_light_ambient", this->ambient);
	shader->setUniform3Array("u_light_positions", (float*)this->positions, n);
	shader->setUniform3Array("u_light_colors", (float*)this->colors, n);
	shader->setUniform3Array("u_light_directions", (float*)this->directions, n);
	shader->setUniform1Array("u_light_types", (int*)this->types, n);
	shader->setUniform2Array("u_light_cos_angles", (float*)this->cos_angles, n);
	shader->setUniform1Array("u_light_intensities", (float*)this->intensities, n);
}