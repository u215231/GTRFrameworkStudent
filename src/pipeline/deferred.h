#pragma once

#include "scene.h"
#include "prefab.h"
#include "light.h"
#include "camera.h"

namespace GFX {
	class FBO;
	class Mesh;
}

namespace SCN {
	class Material;
}

class Deferred {
public:
	GFX::FBO* gbuffer_FBO = nullptr;

	Deferred() = default;
	~Deferred();

	void initGBuffer(int width, int height);
	void resize(int width, int height);
	void bindGBuffer();
	void unbindGBuffer();
	void render(const Matrix44 model, GFX::Mesh* mesh, SCN::Material* material) const;
};

