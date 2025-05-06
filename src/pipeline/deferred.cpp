#include "deferred.h"

#include "camera.h"
#include "../gfx/gfx.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/material.h"


Deferred::~Deferred() {
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
		gbuffer_FBO = nullptr;
	}
}

void Deferred::initGBuffer(int width, int height) {
	if (gbuffer_FBO) {
		delete gbuffer_FBO;
	}

	gbuffer_FBO = new GFX::FBO();

	//Create 2 color attachments with 4 channels and floating point precision
	bool ok = gbuffer_FBO->create(
		width, 
		height,
		2,                  //color_targets
		GL_RGBA,			//format
		GL_UNSIGNED_BYTE,           //type
		true);              //use_depth_texture

	if (!ok) {
		printf("Error: Failed to create G-buffer FBO.\n");
	}
}

void Deferred::resize(int width, int height) {
	if (gbuffer_FBO) {
		gbuffer_FBO->create(width, height, 3, GL_RGBA16F, GL_FLOAT, true);
	}
}

void Deferred::bindGBuffer() {
	if (gbuffer_FBO)
		gbuffer_FBO->bind();
}

void Deferred::unbindGBuffer() {
	if (gbuffer_FBO)
		gbuffer_FBO->unbind();
}


void Deferred::render(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material) const
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	GFX::Shader* shader = NULL;
	Camera* camera = Camera::current;

	glEnable(GL_DEPTH_TEST);

	//chose a shader
	shader = GFX::Shader::Get("gbuffer_fill");

	assert(glGetError() == GL_NO_ERROR);

	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();

	material->bind(shader);

	//upload model matrix
	shader->setUniform("u_model", model);

	//upload camera uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);

	//do the draw call that renders the mesh into the screen
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
