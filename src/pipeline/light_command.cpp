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
	int i = 0;
	for (SCN::LightEntity* light : light_list) {
		this->positions[i] = light->root.getGlobalMatrix().getTranslation();
		this->intensities[i] = light->intensity;
		this->types[i] = light->light_type;
		this->colors[i] = light->color;

		if (light->light_type == SCN::eLightType::DIRECTIONAL) {
			this->directions[i] = light->root.getGlobalMatrix().frontVector();
			this->cos_angle_max[i] = 0.0f;
			this->cos_angle_min[i] = 0.0f;
		}
		else if (light->light_type == SCN::eLightType::SPOT) {
			this->directions[i] = light->root.getGlobalMatrix().frontVector();
			this->cos_angle_max[i] = light->toCos(light->getCosAngleMax());
			this->cos_angle_min[i] = light->toCos(light->getCosAngleMin());
		}
		else {
			this->directions[i] = vec3(0.0f);
			this->cos_angle_min[i] = 0.0f;
			this->cos_angle_max[i] = 0.0f;
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
	shader->setUniform1Array("u_light_cos_angle_max", (float*)this->cos_angle_max, n);
	shader->setUniform1Array("u_light_cos_angle_min", (float*)this->cos_angle_min, n);
	shader->setUniform1Array("u_light_intensities", (float*)this->intensities, n);
}