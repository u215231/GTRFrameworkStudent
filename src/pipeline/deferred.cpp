#include "deferred.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"

Deferred::~Deferred() 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
		gbuffer_FBO = nullptr;
	}
}

void Deferred::initGBuffer(int width, int height) 
{
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
	}
	gbuffer_FBO = new GFX::FBO();
	bool ok = gbuffer_FBO->create(width, height, 3, GL_RGBA, GL_UNSIGNED_BYTE, true);            
	if (!ok) {
		printf("Error: Failed to create G-buffer FBO.\n");
	}
}

void Deferred::resize(int width, int height) 
{
	if (gbuffer_FBO) {
		gbuffer_FBO->create(width, height, 3, GL_RGBA16F, GL_FLOAT, true);
	}
}

void Deferred::bindGBuffer() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->bind();
}

void Deferred::unbindGBuffer() 
{
	if (gbuffer_FBO)
		gbuffer_FBO->unbind();
}


void Deferred::render(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material) const
{
	Camera* camera = Camera::current;

	if (!mesh || !mesh->getNumVertices() || !material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	GFX::Shader* shader = nullptr;
	glEnable(GL_DEPTH_TEST);

	shader = GFX::Shader::Get("gbuffer_fill");

	assert(glGetError() == GL_NO_ERROR);

	if (!shader)
		return;
	shader->enable();

	material->bind(shader);

	shader->setUniform("u_model", model);
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);

	mesh->render(GL_TRIANGLES);

	shader->disable();

	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
