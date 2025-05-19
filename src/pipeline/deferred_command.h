#pragma once

#include "scene.h"

namespace GFX {
	class FBO;
	class Mesh;
	class Texture;
}

namespace SCN {
	class Material;
}

enum class GbufferType {
	ALBEDO_MAP = 0,
	NORMAL_MAP = 1,
	DEPTH_MAP = 2
};

class DeferredCommand {
public:
	int max_textures;
	int width;
	int height;
	GFX::FBO* gbuffer_FBO;

	DeferredCommand();
	~DeferredCommand();

	//gbuffer methods
	void init(int width, int height);
	void resize(int width, int height);
	void bind();
	void unbind();
	void view(GbufferType type);
	void uploadTextures(GFX::Shader* shader) const;
};

