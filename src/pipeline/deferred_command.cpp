#include "deferred_command.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"

DeferredCommand::DeferredCommand()
{
	this->width = 0;
	this->height = 0;
	this->max_textures = 4;
	this->gbuffer_FBO = nullptr;
}

DeferredCommand::~DeferredCommand() 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
		gbuffer_FBO = nullptr;
	}
}

void DeferredCommand::init(int width, int height) 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
	}
	this->width = width;
	this->height = height;
	this->gbuffer_FBO = new GFX::FBO();
	bool status = this->gbuffer_FBO->create(width, height, max_textures, GL_RGBA, GL_UNSIGNED_BYTE, true);
	if (!status) {
		printf("Error: Failed to create G-buffer FBO.\n");
	}
}

// NOT WORKS: fails when setting a new width and height in create method
void DeferredCommand::resize(int width, int height) 
{
	if (!gbuffer_FBO or (this->width == width and this->height == height)) {
		return;
	}
	this->width = width;
	this->height = height;
	bool status = this->gbuffer_FBO->create(width, height, max_textures, GL_RGBA16F, GL_FLOAT, true);
	if (!status) {
		printf("Error: Failed to resize G-buffer FBO.\n");
	}
}

void DeferredCommand::bind() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->bind();
}

void DeferredCommand::unbind() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->unbind();
}

void DeferredCommand::view(GbufferType type)
{
	if (gbuffer_FBO)
		gbuffer_FBO->color_textures[(int)type]->toViewport();
}

void DeferredCommand::uploadTextures(GFX::Shader* shader) const
{
	int texture_slot = 0;
	shader->setTexture("gbuffer_albedo_roughness_map", gbuffer_FBO->color_textures[0], texture_slot++);
	shader->setTexture("gbuffer_normal_metalness_map", gbuffer_FBO->color_textures[1], texture_slot++);
	shader->setTexture("u_gbuffer_position", gbuffer_FBO->color_textures[2], texture_slot++);
	shader->setTexture("u_gbuffer_shadow", gbuffer_FBO->color_textures[3], texture_slot++);

	shader->setTexture("u_gbuffer_depth", gbuffer_FBO->depth_texture, texture_slot++);
}