#include "reflection_probe.h"
#include "../gfx/fbo.h"
#include "../gfx/texture.h"
#include "../gfx/mesh.h"
#include "../gfx/shader.h"
#include "../pipeline/camera.h"
#include "../pipeline/renderer.h"
#include "../pipeline/scene.h"

ReflectionProbeEntity::ReflectionProbeEntity() 
{
    unsigned int w = 512;
    unsigned int h = 512;

    range = 10.0f;
    reflection_strength = 0.5f; 
    texture_unit = 10;
    cubemap = new GFX::Texture();
    cubemap->createCubemap(w, h);

    for (int face = 0; face < NUM_FACES; face++) {
        capture_FBOs[face].create(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, true);
    }
}

ReflectionProbeEntity::~ReflectionProbeEntity() 
{
    if (cubemap) {
        delete cubemap;
        cubemap = nullptr;
    }
}

void ReflectionProbeEntity::setPosition(const vec3& pos)
{
    this->position = pos;
}

void ReflectionProbeEntity::captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer)
{
    Camera cam;
    cam.setPerspective(90.0f, 1.0f, 0.1f, 1000.0f);
    cam.eye = this->position;

    for (int face = 0; face < NUM_FACES; face++) {
        cam.lookAt(cam.eye, cam.eye + directions[face], up_vectors[face]);
        renderer->renderFBO(&cam, &capture_FBOs[face], "texture");
    }
}

void ReflectionProbeEntity::uploadUniforms(GFX::Shader* shader) const
{
    shader->setUniform("u_reflection_probe", cubemap, texture_unit);
    shader->setUniform("u_reflection_strength", reflection_strength); 
    shader->setUniform("u_probe_position", position);
    shader->setUniform("u_probe_range", range);
}

