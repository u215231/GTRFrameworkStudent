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
	this->max_textures = 3;
	this->gbuffer_FBO = nullptr;
}

DeferredCommand::~DeferredCommand() 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
		gbuffer_FBO = nullptr;
	}
}

void DeferredCommand::initGBuffer(int width, int height) 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
	}
	gbuffer_FBO = new GFX::FBO();
	int n = this->max_textures;
	bool status = gbuffer_FBO->create(width, height, n, GL_RGBA, GL_UNSIGNED_BYTE, true);
	if (!status) {
		printf("Error: Failed to create G-buffer FBO.\n");
	}
}

void DeferredCommand::resize(int width, int height) 
{
	if (!gbuffer_FBO) {
		return;
	}
	int n = this->max_textures;
	bool status = gbuffer_FBO->create(width, height, n, GL_RGBA16F, GL_FLOAT, true);
	if (!status) {
		printf("Error: Failed to resize G-buffer FBO.\n");
	}
}

void DeferredCommand::bindGBuffer() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->bind();
}

void DeferredCommand::unbindGBuffer() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->unbind();
}