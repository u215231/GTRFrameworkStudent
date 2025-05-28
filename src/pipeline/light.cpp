#include "light.h"

#include "../core/ui.h"
#include "../utils/utils.h"

SCN::LightEntity::LightEntity()
{
	light_type = eLightType::POINT;
	color.set(1, 1, 1);
	direction.set(0, -1, 0); //Default direction
	cone_info.set(25, 40);
	intensity = 1;
	max_distance = 100;
	cast_shadows = false;
	shadow_bias = 0.001;
	near_distance = 0.1;
	area = 1000;
}

void SCN::LightEntity::configure(cJSON* json)
{
	color = readJSONVector3(json, "color", color );
	intensity = readJSONNumber(json, "intensity", intensity);
	max_distance = readJSONNumber(json, "max_dist", max_distance);
	cast_shadows = readJSONBool(json, "cast_shadows", cast_shadows);

	cone_info.x = readJSONNumber(json, "cone_start", cone_info.x );
	cone_info.y = readJSONNumber(json, "cone_end", cone_info.y );
	area = readJSONNumber(json, "area", area);
	near_distance = readJSONNumber(json, "near_dist", near_distance);

	std::string light_type_str = readJSONString(json, "light_type", "");
	if (light_type_str == "POINT")
		light_type = eLightType::POINT;
	if (light_type_str == "SPOT")
		light_type = eLightType::SPOT;
	if (light_type_str == "DIRECTIONAL")
		light_type = eLightType::DIRECTIONAL;
}

void SCN::LightEntity::serialize(cJSON* json)
{
	writeJSONVector3(json, "color", color);
	writeJSONNumber(json, "intensity", intensity);
	writeJSONNumber(json, "max_dist", max_distance);
	writeJSONBool(json, "cast_shadows", cast_shadows);
	writeJSONNumber(json, "near_dist", near_distance);

	if (light_type == eLightType::SPOT)
	{
		writeJSONNumber(json, "cone_start", cone_info.x);
		writeJSONNumber(json, "cone_end", cone_info.y);
	}
	if (light_type == eLightType::DIRECTIONAL)
		writeJSONNumber(json, "area", area);
	if (light_type == eLightType::POINT)
		writeJSONString(json, "light_type", "POINT");
	if (light_type == eLightType::SPOT)
		writeJSONString(json, "light_type", "SPOT");
	if (light_type == eLightType::DIRECTIONAL)
		writeJSONString(json, "light_type", "DIRECTIONAL");
}

//gets a null camera if light is point or it is not defined
Camera* SCN::LightEntity::getCamera()
{
	if (this->light_type == SCN::eLightType::POINT
		or this->light_type == SCN::eLightType::NO_LIGHT) {
		return nullptr;
	}

	Camera* camera = new Camera();
	
	mat4 light_model = this->root.getGlobalMatrix();
	vec3 light_position = light_model.getTranslation();
	vec3 light_center = light_model * vec3(0.0f, 0.0f, -1.0f);
	vec3 light_up = vec3(0.0f, 1.0f, 0.0f);

	camera->lookAt(light_position, light_center, light_up);

	float half_size = this->area / 2.0f;
	float near_plane = this->near_distance;
	float far_plane = this->max_distance;

	if (this->light_type == SCN::eLightType::DIRECTIONAL) {
		camera->setOrthographic(-half_size, half_size, -half_size, half_size, near_plane, far_plane);
	}
	if (this->light_type == SCN::eLightType::SPOT) {
		camera->setPerspective(2.0f * this->cone_info.y, 1.0f, near_plane, far_plane);
	}

	return camera;
}


