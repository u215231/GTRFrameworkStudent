#include "renderer.h"

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

using namespace SCN;

Renderer::Renderer(const char* shader_atlas_filename)
{
	Vector2 window_size = CORE::getWindowSize();
	unsigned int max_size = max(window_size.x, window_size.y);

	this->render_wireframe = false;
	this->render_boundaries = false;
	this->scene = nullptr;
	this->skybox_cubemap = nullptr;

	for (int i = 0; i < MAX_NUM_LIGHTS; i++) {
		GFX::FBO* shadow_FBO = new GFX::FBO();
		shadow_FBO->setDepthOnly(max_size, max_size);
		this->shadow_FBOs.push_back(shadow_FBO);
	}

	if (!GFX::Shader::LoadAtlas(shader_atlas_filename))
		exit(1);
	GFX::checkGLErrors();

	this->sphere.createSphere(1.0f);
	this->sphere.uploadToVRAM();

	this->is_cubemap_reflections = false;
	this->current_forward_mode = ForwardMode::SINGLE_PASS;
	this->current_pipeline = RenderPipeline::DEFERRED;
	this->lighting_type = Lighting_Type::PBR;
	this->current_gbuffer = GbufferType::ALBEDO_MAP;

	this->deferred_command.init(2 * window_size.x, 2 * window_size.y); // 1024, 768 default
	this->lighting.init(2 * window_size.x, 2 * window_size.y);

	this->environment_cubemap = CubemapFromHDRE("data/panorama.hdre");
	
	this->small_sphere.createSphere(1.0f);
}

Renderer::~Renderer()
{
	if (this->scene) {
		delete this->scene;
		this->scene = nullptr;
	}
	if (this->skybox_cubemap) {
		delete this->skybox_cubemap;
		this->skybox_cubemap = nullptr;
	}
}

void Renderer::setupScene()
{
	if (this->scene->skybox_filename.size()) {
		std::string filename = this->scene->base_folder + "/" + this->scene->skybox_filename;
		this->skybox_cubemap = GFX::Texture::Get(filename.c_str());
	}
	else {
		this->skybox_cubemap = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
// P A R S E R S                                                                   
///////////////////////////////////////////////////////////////////////////////

//store children prefab entities
void Renderer::parseNode(Node* node, Camera* cam)
{
	if (!node) 
		return;

	if (node->mesh) {
		DrawCommand draw_command(node);
		if (node->material->alpha_mode == NO_ALPHA) 
			this->draw_command_opaque_list.push_back(draw_command);
		else 
			this->draw_command_transparent_list.push_back(draw_command);
	}

	for (Node* child : node->children) {
		this->parseNode(child, cam);
	}
}

void Renderer::parsePrefabs(std::vector<PrefabEntity*> prefab_list, Camera* camera)
{
	this->draw_command_opaque_list.clear();
	this->draw_command_transparent_list.clear();
	for (PrefabEntity* prefab : prefab_list) {
		Node* node = &prefab->root;
		this->parseNode(node, camera);
	}

	//sort opaque objects from nearest to farthest
	std::sort(this->draw_command_opaque_list.begin(), this->draw_command_opaque_list.end(),
		[&](const DrawCommand& a, const DrawCommand& b) {
			float distA = (camera->eye - a.model.getTranslation()).length();
			float distB = (camera->eye - b.model.getTranslation()).length();
			return distA < distB;
		});

	//sort transparent objects from farthest to nearest
	std::sort(this->draw_command_transparent_list.begin(), this->draw_command_transparent_list.end(),
		[&](const DrawCommand& a, const DrawCommand& b) {
			float distA = (camera->eye - a.model.getTranslation()).length();
			float distB = (camera->eye - b.model.getTranslation()).length();
			return distA > distB;
		});
}

void Renderer::parseCameraLights(std::vector<SCN::LightEntity*> light_list)
{
	for (Camera* camera : this->camera_light_list) {
		delete camera;
	}
	this->camera_light_list.clear();
	for (LightEntity* light : light_list) {
		Camera* camera = light->getCamera();
		if (camera) {
			this->camera_light_list.push_back(camera);
		}
	}
}

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

void Renderer::parseSceneEntities(Scene* scene, Camera* camera)
{
	this->light_list.clear();
	this->prefab_list.clear();

	for (BaseEntity* entity : scene->entities) {
		if (!entity->visible) {
			continue;
		}

		//store prefab entities
		if (entity->getType() == eEntityType::PREFAB) {
			this->prefab_list.push_back((PrefabEntity*)entity);
			continue;
		}
		//store light entities
		if (entity->getType() == eEntityType::LIGHT) {
			this->light_list.push_back((LightEntity*)entity);
		}
	}

	// For light volumes: still in testing
	std::vector<SCN::LightEntity*> directional_light_list;
	for (SCN::LightEntity* light : this->light_list) {
		if (light->light_type == SCN::eLightType::DIRECTIONAL) {
			directional_light_list.push_back(light);
		}
	}

	this->parsePrefabs(this->prefab_list, camera);

	if (this->current_pipeline == RenderPipeline::LIGHT_VOLUME) {
		this->parseCameraLights(directional_light_list);
		this->light_command.parseLights(directional_light_list, scene);
		this->shadow_command.parseShadows(this->camera_light_list, this->shadow_FBOs);
	}
	else {
		this->parseCameraLights(this->light_list);
		this->light_command.parseLights(this->light_list, scene);
		this->shadow_command.parseShadows(this->camera_light_list, this->shadow_FBOs);
	}
}

///////////////////////////////////////////////////////////////////////////////
// R E N D E R E R S                                                                
///////////////////////////////////////////////////////////////////////////////


//renders the sky box of the scene
void Renderer::renderSkybox(GFX::Texture* cubemap)
{
	Camera* camera = Camera::current;

	//apply skybox necesarry config:
	//no blending, no dpeth test, we are always rendering the skybox
	//set the culling aproppiately, since we just want the back faces
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	if (this->render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GFX::Shader* shader = GFX::Shader::Get("skybox");
	if (!shader)
		return;
	shader->enable();

	//center the skybox at the camera, with a big sphere
	Matrix44 m;
	m.setTranslation(camera->eye.x, camera->eye.y, camera->eye.z);
	m.scale(10, 10, 10);

	//upload camera uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_texture", cubemap, 0);
	shader->setUniform("u_model", m);

	this->sphere.render(GL_TRIANGLES);

	shader->disable();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::renderShaderSinglePass(Camera* camera, DrawCommand draw_command, const char* shader_name) const
{
	if (draw_command.check() || !camera)
		return;

	glEnable(GL_DEPTH_TEST);
	assert(glGetError() == GL_NO_ERROR);

	GFX::Shader* shader = GFX::Shader::Get(shader_name);
	if (!shader)
		return;
	shader->enable();

	draw_command.material->bind(shader);

	this->light_command.uploadUniforms(shader);
	this->shadow_command.uploadUniforms(shader);

	shader->setUniform("u_shininess", draw_command.material->shininess);
	shader->setUniform("u_model", draw_command.model);
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_time", (float)getTime());
	shader->setUniform("u_lighting_type", (int)this->lighting_type);

	if (this->render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	draw_command.mesh->render(GL_TRIANGLES);

	shader->disable();

	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::renderShaderMultiPass(Camera* camera, DrawCommand draw_command, const char* shader_name) const
{
	if (draw_command.check() || !camera)
		return;

	glEnable(GL_DEPTH_TEST);
	assert(glGetError() == GL_NO_ERROR);

	GFX::Shader* shader = GFX::Shader::Get(shader_name);
	if (!shader)
		return;
	shader->enable();

	draw_command.material->bind(shader);

	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	int s = 0;
	for (int i = 0; i < this->light_command.num_lights; i++) {

		if (i == 0)
			glDisable(GL_BLEND);
		else 
			glEnable(GL_BLEND);

		this->light_command.uploadUniform(shader, i);
		if (this->light_command.types[i] != eLightType::POINT) 
			this->shadow_command.uploadUniform(shader, s++);

		shader->setUniform("u_shininess", draw_command.material->shininess);
		shader->setUniform("u_model", draw_command.model);
		shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
		shader->setUniform("u_camera_position", camera->eye);
		shader->setUniform("u_time", (float)getTime());

		if (this->render_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		draw_command.mesh->render(GL_TRIANGLES);
	}

	shader->disable();

	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

//renders the meshes from the point of view of the light camera in textures menu
void Renderer::renderShadow(Camera* light_camera, GFX::FBO* shadow_fbo) const
{
	shadow_fbo->bind();
	glColorMask(false, false, false, false);
	glClear(GL_DEPTH_BUFFER_BIT);

	for (DrawCommand draw_command : this->draw_command_opaque_list) {
		this->renderShaderSinglePass(light_camera, draw_command, "plain");
	}

	glColorMask(true, true, true, true);
	shadow_fbo->unbind();
}

void Renderer::renderForward()
{
	//set the clear color (the background color)
	glClearColor(
		scene->background_color.x,
		scene->background_color.y,
		scene->background_color.z,
		1.0
	);

	//clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	if (this->skybox_cubemap) {
		this->renderSkybox(this->skybox_cubemap);
	}

	for (DrawCommand draw_command : this->draw_command_opaque_list) {
		if (this->current_forward_mode == ForwardMode::SINGLE_PASS) 
			this->renderShaderSinglePass(Camera::current, draw_command, "forward_light_single_pass");
		else 
			this->renderShaderMultiPass(Camera::current, draw_command, "forward_light_multi_pass");
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (DrawCommand draw_command : this->draw_command_transparent_list) {
		if (this->current_forward_mode == ForwardMode::SINGLE_PASS)
			this->renderShaderSinglePass(Camera::current, draw_command, "forward_light_single_pass");
		else
			this->renderShaderMultiPass(Camera::current, draw_command, "forward_light_multi_pass");
	}

	glDisable(GL_BLEND);
}

void Renderer::renderDeferred()
{
	this->deferred_command.bind();

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
		this->renderShaderSinglePass(Camera::current, draw_command, "gbuffer_fill");
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	this->deferred_command.unbind();
}

void Renderer::renderDeferredLightingPass() const {

	Camera* camera = Camera::current;

	//get the full-screen quad mesh
	GFX::Mesh* quad = GFX::Mesh::getQuad();

	//enable the lighting shader
	GFX::Shader* shader = GFX::Shader::Get("deferred_light_pass");
	if (!shader) 
		return;
	shader->enable();

	//send light uniforms
	this->light_command.uploadUniforms(shader);
	this->shadow_command.uploadUniforms(shader);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_inv_vp_mat", camera->viewprojection_matrix.getInverse());

	Vector2 window_size = CORE::getWindowSize();
	shader->setUniform("u_res_inv", vec2(1.0f / window_size.x, 1.0f / window_size.y));
	shader->setUniform("u_lighting_type", (int)this->lighting_type);

	//send geometric buffer textures
	this->deferred_command.uploadTextures(shader);

	//render the full-screen quad
	quad->render(GL_TRIANGLES);

	//disable the shader
	shader->disable();
}


//does not work
void Renderer::renderLightVolumes()
{
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

	lighting.bind();

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
}

void Renderer::renderScene(Scene* scene, Camera* camera)
{
	this->scene = scene;
	if (this->is_cubemap_reflections) {
		return;
	}

	this->setupScene();
	this->parseSceneEntities(scene, camera);

	for (int i = 0; i < (int)this->camera_light_list.size(); i++) {
		Camera* camera_light = this->camera_light_list.at(i);
		GFX::FBO* shadow_fbo = this->shadow_FBOs.at(i);
		this->renderShadow(camera_light, shadow_fbo);
	}

	switch (this->current_pipeline) {
	case RenderPipeline::FORWARD:
		this->renderForward();
		break;
	case RenderPipeline::DEFERRED:
		this->renderDeferred();
		this->renderDeferredLightingPass();
		break;
	// Do not call because does not work
	case RenderPipeline::LIGHT_VOLUME:
		this->renderDeferred();
		this->renderLightVolumes();
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// U S E R   I N T E R F A C E
///////////////////////////////////////////////////////////////////////////////

#ifndef SKIP_IMGUI

void Renderer::showUI()
{
	ImGui::Checkbox("Wireframe", &this->render_wireframe);
	ImGui::Checkbox("Boundaries", &this->render_boundaries);
	
	//shadow bias UI
	static int selected_light = 0;
	int num_shadows = this->shadow_command.num_shadows;
	if (num_shadows > 0) {
		ImGui::Separator();
		ImGui::Text("Shadow Bias Editor");

		//light selector
		ImGui::SliderInt("Shadow Index", &selected_light, 0, num_shadows - 1);

		//clamp selected_light to valid range
		selected_light = std::clamp(selected_light, 0, num_shadows - 1);

		//bias slider
		float& bias = this->shadow_command.biases[selected_light];
		ImGui::SliderFloat("Bias", &bias, 0.0001f, 0.1f, "%.5f");
	}

	//render mode UI
	static int selected_mode = (int)this->current_pipeline;
	ImGui::Separator();
	ImGui::Text("Render Mode Selector\n(0 forward, 1 deferred)");
	ImGui::SliderInt("Render", &selected_mode, 0, 1);
	this->current_pipeline = (RenderPipeline)selected_mode;

	// forward mode UI: single or multi pass
	if (this->current_pipeline == RenderPipeline::FORWARD) {
		static int forward_mode = (int)this->current_forward_mode;
		ImGui::Text("Forward Mode Selector\n(0 single, 1 multi)");
		ImGui::SliderInt("Pass", &forward_mode, 0, 1);
		this->current_forward_mode = (ForwardMode)forward_mode;
	}

	//lighting type UI
	if ((this->current_pipeline == RenderPipeline::FORWARD
	and	this->current_forward_mode != ForwardMode::MULTI_PASS) 
	or this->current_pipeline != RenderPipeline::FORWARD) {
		static int lighting = (int)this->lighting_type;
		ImGui::Text("Lighting Type Selector\n(0 Phong, 1 PBR)");
		ImGui::SliderInt("Lighting", &lighting, 0, 1);
		this->lighting_type = (Lighting_Type)lighting;
	}

	//activate cubemap reflections
	static int cubemap_reflection_mode = (int)this->is_cubemap_reflections;
	ImGui::Separator();
	ImGui::Text("Is Cubemap Mode?");
	ImGui::SliderInt("Cubemap", &cubemap_reflection_mode, 0, 1);
	this->is_cubemap_reflections = (bool)cubemap_reflection_mode;

	//ambient Light Slider
	ImGui::Separator();
	ImGui::SliderFloat("Ambient R", &this->scene->ambient_light.x, 0.0f, 1.0f);
	ImGui::SliderFloat("Ambient G", &this->scene->ambient_light.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Ambient B", &this->scene->ambient_light.z, 0.0f, 1.0f);
}

#else
void Renderer::showUI() {}
#endif
