#pragma once

#include "scene.h"

namespace GFX {
	class FBO;
	class Mesh;
}

namespace SCN {
	class Material;
}

class DeferredCommand {
public:
	int max_textures;
	GFX::FBO* gbuffer_FBO;

	DeferredCommand();
	~DeferredCommand();

	void initGBuffer(int width, int height);
	void resize(int width, int height);
	void bindGBuffer();
	void unbindGBuffer();
};

