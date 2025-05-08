#include "draw_command.h"

DrawCommand::DrawCommand()
{
}

DrawCommand::DrawCommand(GFX::Mesh* mesh, SCN::Material* material, Matrix44 model)
{
	this->mesh = mesh;
	this->material = material;
	this->model = model;
}

DrawCommand::~DrawCommand()
{
}

bool DrawCommand::check() 
{
	return !this->mesh || !this->mesh->getNumVertices() || !this->material;
}