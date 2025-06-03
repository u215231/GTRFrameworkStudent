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

	this->scene = nullptr;
	this->skybox_cubemap = nullptr;

	if (!GFX::Shader::LoadAtlas(shader_atlas_filename))
		exit(1);
	GFX::checkGLErrors();

	this->sphere.createSphere(1.0f);
	this->sphere.uploadToVRAM();

	this->deferred_command.init(window_size.x, window_size.y);

	this->lighting_fbo = new GFX::FBO();
	this->lighting_fbo->create(window_size.x, window_size.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, true);

	this->current_forward_mode = ForwardMode::SINGLE_PASS;
	this->current_pipeline = RenderPipeline::DEFERRED;
	this->lighting_type = Lighting_Type::PBR;
	this->current_gbuffer = GbufferType::ALBEDO_MAP;

	this->render_wireframe = false;
	this->render_boundaries = false;
	this->is_cubemap_reflections = true;
	this->update_ref = false;

	this->probe_grid = new ReflectionProbeGrid();
	this->closest_probe = nullptr;
}

Renderer::~Renderer()
{
	delete this->scene;
	delete this->skybox_cubemap;
	delete this->lighting_fbo;
	delete this->probe_grid;
}

void Renderer::setupScene()
{
	this->skybox_cubemap = this->scene->getSkyboxCubemap();
}

///////////////////////////////////////////////////////////////////////////////
// P A R S E R S                                                                   
///////////////////////////////////////////////////////////////////////////////

//store children prefab entities
void Renderer::parseNode(Node* node, Camera* camera)
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
		this->parseNode(child, camera);
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

void Renderer::parseReflectionProbes(Scene* scene)
{
	for (BaseEntity* entity : scene->entities) {
		if (entity->visible && entity->getType() == eEntityType::REFLECTION_PROBE_GRID)
			this->probe_grid = (ReflectionProbeGrid*)entity;
	}
}

void Renderer::parseSceneEntities(Scene* scene, Camera* camera)
{
	this->light_list.clear();
	this->prefab_list.clear();
	//this->reflection_probes.clear();

	for (BaseEntity* entity : scene->entities) {
		if (!entity->visible) {
			continue;
		}
		switch (entity->getType()) {
			case eEntityType::PREFAB:
				this->prefab_list.push_back((PrefabEntity*)entity); 
				break;
			case eEntityType::LIGHT:
				this->light_list.push_back((LightEntity*)entity); 
				break;
			case eEntityType::REFLECTION_PROBE:
				//this->reflection_probes.push_back((ReflectionProbeEntity*)entity);
				break;
		}
	}

	this->parsePrefabs(this->prefab_list, camera);
	this->light_command.parseLights(this->light_list, scene);
	this->shadow_command.parseShadows(this->light_list);
}

///////////////////////////////////////////////////////////////////////////////
// R E N D E R E R S                                                                
///////////////////////////////////////////////////////////////////////////////

//renders the sky box of the scene
void Renderer::renderSkybox(Camera* camera, GFX::Texture* cubemap)
{
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

void Renderer::renderFBO(Camera* camera, GFX::FBO* fbo, const char* shader_name)
{
	fbo->bind();

	glClearColor(
		this->scene->background_color.x,
		this->scene->background_color.y,
		this->scene->background_color.z,
		1.0
	);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	if (this->skybox_cubemap) {
		this->renderSkybox(camera, this->skybox_cubemap);
	}

	for (DrawCommand draw_command : this->draw_command_opaque_list) {
		this->renderShaderSinglePass(camera, draw_command, shader_name);
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	fbo->unbind();
}

void Renderer::renderFBO_Deferred(Camera* camera, GFX::FBO* fbo, const char* shader_name)
{
	fbo->bind();

	glClearColor(
		this->scene->background_color.x,
		this->scene->background_color.y,
		this->scene->background_color.z,
		1.0
	);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GFX::checkGLErrors();

	if (this->skybox_cubemap) {
		this->renderSkybox(camera, this->skybox_cubemap);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->renderFBO(camera, this->deferred_command.gbuffer_FBO, "gbuffer_fill");

	//get the full-screen quad mesh
	GFX::Mesh* quad = GFX::Mesh::getQuad();

	this->deferred_command.gbuffer_FBO->depth_texture->copyTo(this->lighting_fbo->depth_texture);

	this->lighting_fbo->bind();

	//enable the lighting shader
	GFX::Shader* shader = GFX::Shader::Get(shader_name);
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
	shader->setUniform("u_pbr_probes", (int)this->is_cubemap_reflections);

	//send geometric buffer textures
	this->deferred_command.uploadTextures(shader);

	//glDisable(GL_DEPTH_TEST);

	//render the full-screen quad
	quad->render(GL_TRIANGLES);
	glEnable(GL_DEPTH_TEST);

	//disable the shader
	shader->disable();

	this->lighting_fbo->unbind();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	fbo->unbind();
	this->lighting_fbo->color_textures[0]->toViewport();
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
		this->renderSkybox(Camera::current, this->skybox_cubemap);
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

	//added temporary
	//this->renderSphere();

	glDisable(GL_BLEND);
}

void Renderer::renderDeferred() 
{
	Camera* camera = Camera::current;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->renderFBO(camera, this->deferred_command.gbuffer_FBO, "gbuffer_fill");

	//get the full-screen quad mesh
	GFX::Mesh* quad = GFX::Mesh::getQuad();

	this->deferred_command.gbuffer_FBO->depth_texture->copyTo(this->lighting_fbo->depth_texture);

	this->lighting_fbo->bind();

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
	shader->setUniform("u_pbr_probes", (int)this->is_cubemap_reflections);
	
	//send geometric buffer textures
	this->deferred_command.uploadTextures(shader);

	//send cubemap reflections
	//this->reflection_probe->uploadUniforms(shader);
	vec3 camera_pos = Camera::current->eye;
	float best_dist = FLT_MAX;

	for (ReflectionProbeEntity* probe : this->probe_grid->reflection_probes) {
		float dist = (probe->position - camera_pos).length();
		if (dist <= best_dist) {
			best_dist = dist;
			this->closest_probe = probe;
		}
	}

	if (this->closest_probe != nullptr) {
		this->closest_probe->uploadUniforms(shader);
	}


	//glDisable(GL_DEPTH_TEST);

	//render the full-screen quad
	quad->render(GL_TRIANGLES);
	glEnable(GL_DEPTH_TEST);

	//disable the shader
	shader->disable();

	if (this->probe_grid->render_spheres_mode) {
		this->probe_grid->renderSpheres(this);
	}

	this->lighting_fbo->unbind();
	this->lighting_fbo->color_textures[0]->toViewport();
}

void Renderer::renderScene(Scene* scene, Camera* camera)
{
	this->scene = scene;
	this->setupScene();
	this->parseSceneEntities(scene, camera);

	this->shadow_command.renderShadows(this);

	switch (this->current_pipeline) {
		case RenderPipeline::FORWARD:
			this->renderForward();
			break;
		case RenderPipeline::DEFERRED:
			this->renderDeferred();
			break;
		// Do not call because does not work
		case RenderPipeline::LIGHT_VOLUME:
			//this->renderDeferred();
			//this->renderLightVolumes();
			break;
	}

	if (this->update_ref) {
		for (ReflectionProbeEntity* probe : this->probe_grid->reflection_probes) {
			probe->captureEnvironment(scene, this);
		}
		this->update_ref = false;
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
		ImGui::SliderInt("Light Pass", &forward_mode, 0, 1);
		this->current_forward_mode = (ForwardMode)forward_mode;
	}

	//lighting type UI
	if ((this->current_pipeline == RenderPipeline::FORWARD
		and this->current_forward_mode != ForwardMode::MULTI_PASS)
		or this->current_pipeline != RenderPipeline::FORWARD) {
		static int lighting = (int)this->lighting_type;
		ImGui::Text("Lighting Type Selector\n(0 Phong, 1 PBR)");
		ImGui::SliderInt("Lighting", &lighting, 0, 1);
		this->lighting_type = (Lighting_Type)lighting;
	}

	//ambient Light Slider
	ImGui::Separator();
	ImGui::Text("Ambient Light");
	ImGui::SliderFloat("Ambient R", &this->scene->ambient_light.x, 0.0f, 1.0f);
	ImGui::SliderFloat("Ambient G", &this->scene->ambient_light.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Ambient B", &this->scene->ambient_light.z, 0.0f, 1.0f);

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

	//activate cubemap reflections
	static int cubemap_reflection_mode = (int)this->is_cubemap_reflections;
	ImGui::Separator();
	ImGui::Text("Is Reflection Probes Mode?");
	ImGui::SliderInt("Cubemap", &cubemap_reflection_mode, 0, 1);
	this->is_cubemap_reflections = (bool)cubemap_reflection_mode;

	//Capture reflections updater
	ImGui::Separator();
	if (ImGui::Button("Update Reflections")) {
		this->update_ref = true;
	}

	//Render Spheres toggle
	static int render_spheres = (int)this->probe_grid->render_spheres_mode;
	ImGui::Separator();
	ImGui::Text("Render Spheres Toggle\n(0 No Spheres, 1 Spheres)");
	ImGui::SliderInt("Render Spheres?", &render_spheres, 0, 1);
	this->probe_grid->render_spheres_mode = (bool)render_spheres;

	//reflection strenght slider
	float strength = this->closest_probe->reflection_strength;
	ImGui::Separator();
	ImGui::Text("Reflection Strength");
	ImGui::SliderFloat("Strength", &strength, 0.0f, 50.0f);
	this->closest_probe->reflection_strength = strength;
}

#else
void Renderer::showUI() {}
#endif
