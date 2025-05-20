#pragma once

#include "scene.h"

namespace SCN {
	class Material;
	class Node;
}

namespace GFX {
	class Mesh;
	class Shader;
	
}

class DrawCommand
{
public:
	GFX::Mesh* mesh = nullptr;
	SCN::Material* material = nullptr;
	Matrix44 model;

	DrawCommand();
	DrawCommand(GFX::Mesh* mesh, SCN::Material* material, Matrix44 model);
	DrawCommand(SCN::Node* node);
	~DrawCommand();

	bool check();
};